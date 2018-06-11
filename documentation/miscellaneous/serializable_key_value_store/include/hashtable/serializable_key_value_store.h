/**
 * AOS Class 2017 Group C -- Individual Project: Nameserver Gaia
 *                           Serializable key value store
 *
 * Implements a serializable key value store for string keys and values.
 *
 * Uses the provided hashtable implementation. However, it does not offer all
 * possible types of the hashtable/dictionary at the moment. Only strings.
 *
 * NOTE: As in the hashtable implementation, the store does not allocate
 * actual memory for the strings. Only pointers are stored.
 * Thus, the client of this interface keeps memory ownership!
 *
 * EXCEPT: during serialization, the size is auto-calculated and a buffer is
 * for the serialized store is allocated with malloc() in case of success.
 * Thus, the user must free() this buffer afterwards.
 *
 * In addition to the regular hashmap, it offers an iterate() function
 * that allows applying a lambda function over all elements of the hashmap.
 *
 * In addition to the regular hashmap with put() that would introduce
 * duplicates instead of updating values with identical key, also
 * a set() function is offered here that can update already existing keys.
 *
 * version 2017-11-29, Pirmin Schmid
 */

#ifndef INCLUDE_HASHTABLE_SERIALIZABLE_KEY_VALUE_STORE_H_
#define INCLUDE_HASHTABLE_SERIALIZABLE_KEY_VALUE_STORE_H_

#include <hashtable/hashtable.h>

struct serializable_key_value_store {
    struct hashtable *ht;
    void *buf; // is a serialized key value store if created from serialized store (NULL otherwise)
               // needed for proper / easier cleanup when stores were created after deserialization
};

struct serialized_key_value_store {
    size_t buf_size;
    size_t capacity;
    size_t entries;
    size_t key_size;
    size_t value_size;
    char buf[0];
};

/**
 * \brief create an empty hashtable with a given capacity and load factor
 * \param capacity the capacity
 * \return an empty hashtable in case of success; NULL otherwise.
 *
 * NOTE: As seen in the implementation of the hashtable, capacity corresponds
 * more to a "bucket nr" at the moment.
 */
struct serializable_key_value_store *create_kv_store(size_t capacity);

/** Added
 * \brief cleans up all hashtable allocations
 * \param *store is set to NULL after cleanup
 *
 * NOTE: as in all hashtable operations in the provided implementation used for the store,
 * the memory of keys and values is not touched. The hashtable never takes ownership about that.
 * Thus, the client needs to track/cleanup that memory on its own.
 */
void destroy_kv_store(struct serializable_key_value_store **store);

/**
 * The same as destroy_kv_store(), but does not touch the embedded serialized buffer.
 */
void destroy_kv_store_borrowed_buffer(struct serializable_key_value_store **store);

/**
 * \brief deserialize a store
 * \param serialized_store
 * \param min_additional_entries  assures that new capacity is MAX(capacity, size + min_additional_entries)
 *
 * NOTE: this function uses directly the keys and values inside of the serialized store.
 * Thus: do NOT free the serialized_store after calling this function.
 * This matches the semantics of the rest of the hashtable functions in the provided implementation.
 * Provided memory spaces are NOT to be freed.
 *
 * NOTE: destroy_kv_store() will then also free the embedded serialized buffer that was used
 * for the deserialization. Thus, no memory leak.
 *
 * \return deserialized store or NULL if not successful
 */
struct serializable_key_value_store *deserialize_kv_store(const struct serialized_key_value_store *serialized_store,
                                                          size_t min_additional_entries);


/**
 * \brief Serializes a store
 * \param store
 *
 * NOTE: this function allocates the memory used for serialization and returns it.
 * Use free() later if the serialized store is not used anymore.
 * The store itself remains unchanged and can be used after this operation if desired
 * and should be destroyed after use.
 *
 * \return serialized store or NULL if not successful
 */
struct serialized_key_value_store *serialize_kv_store(const struct serializable_key_value_store *store);


/**
 * \brief put a new key/value pair into the hashtable (compatibility mode with hashtable)
 * \param store
 * \param key     Must be a string.
 * \param value   Must be a string.
 *
 * NOTE: This function does not copy the key nor the value for storage.
 * The caller is responsible for maintaining the value, the hashtable only keeps pointers.
 *
 * NOTE: To keep the semantics with the implementation in hashtable.c,
 * this function does *not* check whether the identical key already exists,
 * and directly inserts a potential duplicate.
 *
 * Since a typical key-value store needs to be updateable, an improved version
 * is offered below _set(), that fulfills this semantics. Please note, it re-uses
 * some code that is written in the actual get and also the actual put functions.
 * But this is faster than calling get(), then potentially remove() and then put().
 *
 * \return 0 if the operation succeeded, otherwise an error code.
 */
static inline int kv_store_put(struct serializable_key_value_store *store, const char *key, const char *value)
{
    assert(store);
    assert(key);
    assert(value);

    return store->ht->d.put_word(&store->ht->d, key, strlen(key), (uintptr_t)value);
}


/**
 * \brief set a new key/value pair into the hashtable (checks whether key exists and performs update then)
 * \param store
 * \param key     Must be a string.
 * \param value   Must be a string.
 *
 * NOTE: This function does not copy the key nor the value for storage.
 * The caller is responsible for maintaining the value, the hashtable only keeps pointers.
 *
 * NOTE: In contrast to _put(), this function checks whether a key already exists
 * and updates the content in that case instead of inserting a new value.
 * This corresponds to the typical behavior of such key value stores.
 *
 * This function uses some code from hashtable implementation.
 *
 * \return 0 if the operation succeeded, otherwise an error code.
 */
int kv_store_set(struct serializable_key_value_store *store, const char *key, const char *value);


/**
 * \brief get a value from the hashtable for a given key
 * \param store
 * \param key    Has to be a zero-terminated string.
 *
 * \return the value or NULL if there is no such key/value pair
 *         Note: this is a pointer to a string. No copy. No ownership transfer.
 */
static inline const char *kv_store_get(struct serializable_key_value_store *store, const char *key)
{
    assert(store);
    assert(key);

    char *ret_value = NULL;
    store->ht->d.get(&store->ht->d, key, strlen(key), (void **)&ret_value);
    return ret_value;
}


/**
 * \brief remove a key-value pair from the hashtable for a given key
 * \param store
 * \param key    Has to be a zero-terminated string.
 *
 * NOTE: This is a no-op in case the key was not found.
 *       As in all hashtable operations (see implementation there), this only removes the
 *       internal data structures in the hashtable but does not free key nor value memory.
 *       The store / hashtable never takes ownership.
 */
static inline void kv_store_remove(struct serializable_key_value_store *store, const char *key)
{
    assert(store);
    assert(key);

    store->ht->d.remove(&store->ht->d, key, strlen(key));
}


static inline size_t kv_store_size(const struct serializable_key_value_store *store)
{
    assert(store);
    return store->ht->entry_count;
}

static inline size_t kv_store_capacity(const struct serializable_key_value_store *store)
{
    assert(store);
    return store->ht->capacity;
}

/**
 * \brief Iteration over the entire hashmap using a lambda function that can take
 * an additional arbitrary argument.
 */
typedef void (*kv_store_lambda_function_t)(const struct _ht_entry *entry, void *arg);

void kv_store_iterate(const struct serializable_key_value_store *store,
                      kv_store_lambda_function_t lambda, void *arg);


/**
 * \brief Allows a quick print functionality for the KV store.
 * More sophisticated functions can be built accordingly.
 * See the kv_store_iterate() function.
 */
void fprint_kv_store(FILE *stream, const struct serializable_key_value_store *store);

static inline void print_kv_store(const struct serializable_key_value_store *store)
{
    fprint_kv_store(stdout, store);
}

#endif // INCLUDE_HASHTABLE_SERIALIZABLE_KEY_VALUE_STORE_H_
