/**
 * Implements a serializable key value store for string keys and values.
 *
 * See header file for details
 *
 * version 2017-11-28, Pirmin Schmid
 */

#include <hashtable/serializable_key_value_store.h>

// note: the current hashtable is not even using this value (corresponding to 0.75)
#define HASHTABLE_DEFAULT_LOAD_FACTOR 75

struct serializable_key_value_store *create_kv_store(size_t capacity)
{
    struct serializable_key_value_store *store = calloc(1, sizeof(*store));
    if (!store) {
        return store;
    }

    store->ht = create_hashtable2(capacity, HASHTABLE_DEFAULT_LOAD_FACTOR);
    if (!store->ht) {
        free(store);
        return NULL;
    }

    store->buf = NULL;
    return store;
}

void destroy_kv_store(struct serializable_key_value_store **store)
{
    assert(store);
    struct serializable_key_value_store *s = *store;

    if (!s) {
        return;
    }

    destroy_hashtable(&s->ht);

    if (s->buf) {
        free(s->buf);
    }

    free(s);
    *store = NULL;
}

void destroy_kv_store_borrowed_buffer(struct serializable_key_value_store **store)
{
    assert(store);
    struct serializable_key_value_store *s = *store;

    if (!s) {
        return;
    }

    destroy_hashtable(&s->ht);

    // does not touch the buffer

    free(s);
    *store = NULL;
}


struct serializable_key_value_store *deserialize_kv_store(const struct serialized_key_value_store *serialized_store,
                                                          size_t min_additional_entries)
{
    assert(serialized_store);

    size_t capacity = serialized_store->entries + min_additional_entries;
    if (capacity < serialized_store->capacity) {
        capacity = serialized_store->capacity;
    }
    struct serializable_key_value_store *store = create_kv_store(capacity);
    if (!store) {
        return NULL;
    }

    const size_t n = serialized_store->entries;
    const char *buf = serialized_store->buf;
    size_t offset = 0;
    const size_t key_size = serialized_store->key_size;
    const size_t entry_size = serialized_store->value_size + key_size;
    for (size_t i = 0; i < n; i++) {
        const char *key = buf + offset;
        const char *value = key + key_size;
        int e = kv_store_put(store, key, value);
        if (e != 0) {
            goto error_exit;
        }
        offset += entry_size;
    }

    store->buf = (void *)serialized_store;
    return store;

error_exit:
    destroy_kv_store_borrowed_buffer(&store);
    return NULL;
}

struct max_len {
    size_t keys;
    size_t values;
};

static void get_max(const struct _ht_entry *entry, void *arg)
{
    struct max_len *m = arg;
    if (m->keys < entry->key_len) {
        m->keys = entry->key_len;
    }
    size_t len = strlen(entry->value);
    if (m->values < len) {
        m->values = len;
    }
}

struct buf_offset {
    char *buf;
    size_t value_offset;
    size_t entry_offset;
};

static void copy_to_buf(const struct _ht_entry *entry, void *arg)
{
    struct buf_offset *b = arg;
    strcpy(b->buf, entry->key);
    strcpy(b->buf + b->value_offset, entry->value);
    b->buf += b->entry_offset;
}

struct serialized_key_value_store *serialize_kv_store(const struct serializable_key_value_store *store)
{
    assert(store);

    // determine needed buffer size. Note: to keep it simple, just a max size
    // is determined for keys and for values and then space is reserved as if all
    // keys have equal size and all values have equal size.
    // This is a simplification but it simplifies deserialize quite a lot.

    struct max_len max = { .keys = 0, .values = 0 };
    kv_store_iterate(store, get_max, &max);
    // add 0 terminators
    max.keys++;
    max.values++;

    size_t size = sizeof(struct serialized_key_value_store);
    size += (max.keys + max.values) * kv_store_size(store);

    struct serialized_key_value_store *buffer = calloc(1, size);
    if (!buffer) {
        return NULL;
    }

    buffer->buf_size = size;
    buffer->capacity = kv_store_capacity(store);
    buffer->entries = kv_store_size(store);
    buffer->key_size = max.keys;
    buffer->value_size = max.values;
    struct buf_offset b = { .buf = buffer->buf, .value_offset = max.keys, .entry_offset = max.keys + max.values };
    kv_store_iterate(store, copy_to_buf, &b);
    return buffer;
}


/**
 * These 3 functions/macros are from Hashtable
 */
static inline int hash(const char *str, size_t key_len)
{
    register int _hash = 5381;
    register int _c;

    for(size_t i = 0; i < key_len; i++) {
        _c = str[i];
        _hash = ((_hash << 5) + _hash) + _c;
    }
    return _hash;
}

static inline int index_for(int table_length, int hash_value)
{
    return ((unsigned)hash_value % table_length);
}

#define equals(_k1, _k2, len) (!memcmp((_k1), (_k2), (len)))

int kv_store_set(struct serializable_key_value_store *store, const char *key, const char *value)
{
    assert(store);
    assert(key);
    assert(value);

    struct hashtable *ht = store->ht;

    size_t key_len = strlen(key);
    int _hash_value = hash(key, key_len);
    int _index = index_for(ht->table_length, _hash_value);
    struct _ht_entry *_e = ht->entries[_index];

    while (NULL != _e) {
        if ((_hash_value == _e->hash_value) && (equals(key, _e->key, key_len))) {
            assert(_e->type != TYPE_CAPABILITY);
            // update entry
            _e->value = (void *)value;
            return 0;
        }
        _e = _e->next;
    }

    // not existing
    return kv_store_put(store, key, value);
}


void kv_store_iterate(const struct serializable_key_value_store *store,
                      kv_store_lambda_function_t lambda, void *arg)
{
    assert(store);
    assert(lambda);
    // arg is optional

    size_t n = store->ht->table_length;
    for (size_t i = 0; i < n; i++) {
        struct _ht_entry *e = store->ht->entries[i];
        while (e) {
            lambda(e, arg);
            e = e->next;
        }
    }
}

static void fprint_entry(const struct _ht_entry *entry, void *arg)
{
    FILE *stream = arg;

    // no error check
    fprintf(stream, "%s =>  %s\n", entry->key, entry->value);
}

void fprint_kv_store(FILE *stream, const struct serializable_key_value_store *store)
{
    assert(stream);
    assert(store);

    // no error check
    fprintf(stream, "Key value store:\n");
    kv_store_iterate(store, fprint_entry, stream);
}
