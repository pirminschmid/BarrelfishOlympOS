/**
 * \file
 * \brief Hashtable headers
 */

/*
 * Copyright (c) 2008, 2011, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <hashtable/dictionary.h>
#include <stdio.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

/**
 * \brief an entry of a hashtable
 */
struct _ht_entry {
    const void* key;
    size_t key_len;
    void* value;
    struct capref capvalue;
    ENTRY_TYPE type;
    int hash_value;
    struct _ht_entry *next;
};

/**
 * \brief hashtable
 */
struct hashtable {
    struct dictionary d;
    int table_length;
    int entry_count;
    struct _ht_entry **entries;
    int threshold;
    int capacity;
    int load_factor;
};

/**
 * \brief create an empty hashtable with a given capacity and load factor
 * \param capacity the capacity
 * \param the load factor
 * \return an empty hashtable.
 */
struct hashtable* create_hashtable2(int capacity, int loadFactor);

/**
 * \brief create an empty hashtable with default capacity and load factor
 * \return an empty hashtable
 */
struct hashtable* create_hashtable(void);


/** Added
 * \brief cleans up all hashtable internal allocations
 * \param *ht is set to NULL after cleanup
 *
 * NOTE: as in all hashtable operations in the provided implementation,
 * the memory of keys and values is not touched. The hashtable never takes ownership about that.
 * Thus, the client needs to track/cleanup that memory on its own.
 */
void destroy_hashtable(struct hashtable **ht);

// is not implemented
void print_hashtable(FILE *stream, struct hashtable *ht);

__END_DECLS

#endif /*HASHTABLE_H_*/
