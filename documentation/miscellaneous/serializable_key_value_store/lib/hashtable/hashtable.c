/**
 * \file hashtable.c
 * \brief Hashtable implementation
 */

/*
 * Copyright (c) 2008,2009,2011 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

/**
 * AOS class 2017: Group C
 * Fixed bug:
 * - entry_count was not set to 0
 * - adjustment of entry_count during remove
 *
 * Suspected bug:
 * - Currently, _ht->table_length * sizeof(struct _ht_entry) is allocated/memset for entries.
 *   From the code, _ht->table_length * sizeof(struct _ht_entry *) should be sufficient.
 *   Only pointers are stored in the array and not the actual structs.
 *   -> left unchanged at the moment since not a real problem
 *      and hard to test overflows at the moment and maybe it was meant
 *      for additional concepts
 *
 * Extension:
 * - cleanup/destroy method
 *
 * version 2017-11-29 Pirmin Schmid
 */
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <hashtable/hashtable.h>
#include <hashtable/multimap.h>

/**
 * \brief get a hash value for a string
 * \param str the string
 * \return the hash value
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

/**
 * \brief get the index for an given hash in the bucket table
 * \param table_length the length of the table
 * \param hash_value the hash
 * \return the index
 */
static inline int index_for(int table_length, int hash_value)
{
    return ((unsigned)hash_value % table_length);
}

/**
 * \brief check two keys for equality
 * \param k1 the first string
 * \param k2 the second string
 * \return true if the strings are equal
 */
#define equals(_k1, _k2, len) (!memcmp((_k1), (_k2), (len)))


/**
 * \brief get the number of entries in a hashtable
 * \param h the hashtable
 * \return the number of entries
 */
static int ht_size(struct dictionary *dict)
{
    assert(dict != NULL);
    struct hashtable *ht = (struct hashtable*) dict;

    return ht->entry_count;
}

/**
 * \brief put a new key/value pair into the hashtable
 * \param ht the hashtable
 * \param key the key. Has to be a string.
 * \param value the value. Can be any pointer. This function does
 *      not copy the value or stores it. The caller is responsible for
 *      maintaining the value, the hashtable only keeps pointers.
 * \return 0 if the operation succeeded, otherwise an error code.
 */
static int ht_put(struct dictionary *dict, struct _ht_entry *entry)
{
    assert(dict != NULL);
    struct hashtable *ht = (struct hashtable*) dict;

    int _hash_value = hash(entry->key, entry->key_len);

    // TODO: XXX check for size and increase capacity, if necessary
    ++(ht->entry_count);

    entry->hash_value = _hash_value;
    int _index = index_for(ht->table_length, _hash_value);
    entry->next = ht->entries[_index];
    ht->entries[_index] = entry;
    return 0;
}

static int ht_put_word(struct dictionary *dict, const char *key, size_t key_len,
                       uintptr_t value)
{
    struct _ht_entry *e = malloc(sizeof(struct _ht_entry));
    if (NULL == e) {
        return 1;
    }
    e->key = key;
    e->key_len = key_len;
    e->value = (void*) value;
    e->type = TYPE_WORD;

    return ht_put(dict, e);
}

static int ht_put_capability(struct dictionary *dict, char *key,
                             struct capref cap)
{
    struct _ht_entry *e = malloc(sizeof(struct _ht_entry));
    if (NULL == e) {
        return 1;
    }
    e->key = key;
    e->key_len = strlen(key);
    e->capvalue = cap;
    e->type = TYPE_CAPABILITY;

    return ht_put(dict, e);
}

/**
 * \brief get a value from the hashtable for a given key
 * \param ht the hashtable
 * \param key the key. Has to be a zero-terminated string.
 * \return the value or NULL if there is no such key/value pair
 */
static ENTRY_TYPE ht_get(struct dictionary *dict, const char *key, size_t key_len,
                         void **value)
{
    assert(dict != NULL);
    assert(key != NULL);
    assert(value != NULL);

    struct hashtable *ht = (struct hashtable*) dict;

    int _hash_value = hash(key, key_len);
    int _index = index_for(ht->table_length, _hash_value);
    struct _ht_entry *_e = ht->entries[_index];

    while (NULL != _e) {
        if ((_hash_value == _e->hash_value) && (equals(key, _e->key, key_len))) {
            assert(_e->type != TYPE_CAPABILITY);
            *value = _e->value;
            return _e->type;
        }
        _e = _e->next;
    }
    *value = NULL;
    return 0;
}

static ENTRY_TYPE ht_get_capability(struct dictionary *dict, char *key,
                                    struct capref *value)
{
    assert(dict != NULL);
    assert(key != NULL);
    assert(value != NULL);

    struct hashtable *ht = (struct hashtable*) dict;
    size_t key_len = strlen(key);
    int _hash_value = hash(key, key_len);
    int _index = index_for(ht->table_length, _hash_value);
    struct _ht_entry *_e = ht->entries[_index];

    while (NULL != _e) {
        if ((_hash_value == _e->hash_value) && (equals(key, _e->key, key_len))) {
            assert(_e->type == TYPE_CAPABILITY);
            *value = _e->capvalue;
            return _e->type;
        }
        _e = _e->next;
    }
    *value = NULL_CAP;
    return 0;
}

static int ht_remove(struct dictionary *dict, const char *key, size_t key_len)
{
    assert(dict != NULL);
    struct hashtable *ht = (struct hashtable*) dict;

    int _hash_value = hash(key, key_len);
    int _index = index_for(ht->table_length, _hash_value);
    struct _ht_entry *_e = ht->entries[_index];
    struct _ht_entry *_prev = NULL;
    while (NULL != _e) {
        if ((_hash_value == _e->hash_value) && (equals(key, _e->key, key_len))) {
            if (_prev == NULL) {
                ht->entries[_index] = _e->next;
            } else {
                _prev->next = _e->next;
            }
            free(_e);
            --(ht->entry_count);
            return 0;
        }
        _prev = _e;
        _e = _e->next;
    }
    return 1;
}

// DONE: XXX implement destructors
void destroy_hashtable(struct hashtable **ht)
{
    assert(ht);
    assert(*ht);

    struct hashtable *_ht = *ht;

    // entries
    size_t n = _ht->table_length;
    for (size_t i = 0; i < n; i++) {
        struct _ht_entry *e = _ht->entries[i];
        while (e) {
            struct _ht_entry *to_remove = e;
            e = e->next;
            free(to_remove);
        }
    }

    // entries table
    free(_ht->entries);

    // hashtable
    free(_ht);

    *ht = NULL;
}

/**
 * \brief create an empty hashtable with a given capacity and load factor
 * \param capacity the capacity
 * \param the load factor
 * \return an empty hashtable.
 */
static void ht_init(struct hashtable *_ht, int capacity, int load_factor)
{
    _ht->capacity = capacity;
    _ht->load_factor = load_factor;
    _ht->table_length = capacity;
    _ht->entry_count = 0;
    _ht->entries = malloc(_ht->table_length * sizeof(struct _ht_entry));
    assert(_ht->entries != NULL);
    memset(_ht->entries, 0, _ht->table_length * sizeof(struct _ht_entry));
    _ht->threshold = (capacity * load_factor) / 100;
    _ht->d.size = ht_size;
    _ht->d.put_word = ht_put_word;
    _ht->d.put_capability = ht_put_capability;
    _ht->d.get = ht_get;
    _ht->d.get_capability = ht_get_capability;
    _ht->d.remove = ht_remove;
}

// XXX TODO: loadFactor should be a float, 0.75 instead of 75
struct hashtable* create_hashtable2(int capacity, int load_factor)
{
    struct hashtable *_ht = malloc(sizeof(struct hashtable));
    assert(_ht != NULL);
    ht_init(_ht, capacity, load_factor);
    return _ht;
}


/**
 * \brief create an empty hashtable with default capacity and load factor
 * \return an empty hashtable
 */
struct hashtable* create_hashtable(void)
{
    return create_hashtable2(11, 75);
}
