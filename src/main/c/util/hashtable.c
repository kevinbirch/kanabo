/*
 * Copyright (c) 2013 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 *
 * Distributed under an [MIT-style][license] license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimers in the documentation and/or
 *   other materials provided with the distribution.
 * - Neither the names of the copyright holders, nor the names of the authors, nor
 *   the names of other contributors may be used to endorse or promote products
 *   derived from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/ncsa
 */

#include <tgmath.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "hashtable.h"

static const float  DEFAULT_LOAD_FACTOR = 0.75f;
static const size_t DEFAULT_CAPACITY = 8ul;

static const uint8_t * const CHAINED_KEY = (uint8_t *)"CHAINED_KEY_SENTINEL";

struct hashtable_s
{
    /** count of keys stored in the table */
    size_t        occupied;
    /** the number of keys that can be stored at the current table size */
    size_t        capacity;
    /** the hashing overhead to accommodate */
    float         load_factor;
    /** can this hashtable be modified? */
    bool          mutable;

    /** the key hashing function */
    hash_function    hash;
    /** the key comparison function */
    compare_function compare;

    /** the length of the entries table */
    size_t    length;
    /** the keys and values table */
    uint8_t **entries;
};

struct chain_s
{
    size_t   length;
    uint8_t *entries[];
};

typedef struct chain_s Chain;

struct item_adapter_s
{
    hashtable_item_iterator iterator;
    void *original_context;
};

typedef struct item_adapter_s item_adapter;

struct equality_adapter_s
{
    const Hashtable *other;
    compare_function compare;
};

typedef struct equality_adapter_s equality_adapter;

static Hashtable *alloc(size_t size);
static inline void alloc_table(Hashtable *hashtable, size_t size);
static void init(Hashtable *hashtable, compare_function comparitor, size_t size, float load_factor, hash_function function);
static inline size_t normalize_capacity(size_t hint);

static bool chained_contains(const Hashtable *hashtable, size_t index, const void *key);
static void *chained_get(const Hashtable *hashtable, size_t index, const void *key);

static void *add_to_chain(Hashtable *hashtable, size_t index, void *key, void *value);
static void expand_chain(Hashtable *hashtable, size_t index, void *key, void *value);
static void *chained_put(Hashtable *hashtable, size_t index, void *key, void *value);
static void *chained_remove(Hashtable *hashtable, size_t index, void *key);

static void rehash(Hashtable *hashtable);

static bool key_item_iterator(void *key, void *value __attribute__((unused)), void *context);
static bool value_item_iterator(void *key __attribute__((unused)), void *value, void *context);
static bool map_into(void *key, void *value, void *context);
static bool contains_key_value(void *key, void *value, void *context);

static inline size_t hash_index(const Hashtable *hashtable, const void *key);

Hashtable *make_hashtable(compare_function comparitor)
{
    return make_hashtable_with_capacity_factor(comparitor, DEFAULT_CAPACITY, DEFAULT_LOAD_FACTOR);
}

Hashtable *make_hashtable_with_function(compare_function comparitor,
                                        hash_function function)
{
    return make_hashtable_with_capacity_factor_function(comparitor, DEFAULT_CAPACITY, DEFAULT_LOAD_FACTOR, function);
}

Hashtable *make_hashtable_with_capacity(compare_function comparitor,
                                        size_t capacity_hint)
{
    return make_hashtable_with_capacity_factor(comparitor, capacity_hint, DEFAULT_LOAD_FACTOR);
}

Hashtable *make_hashtable_with_capacity_function(compare_function comparitor,
                                                 size_t capacity_hint,
                                                 hash_function function)
{
    return make_hashtable_with_capacity_factor_function(comparitor, capacity_hint, DEFAULT_LOAD_FACTOR, function);
}

Hashtable *make_hashtable_with_capacity_factor(compare_function comparitor,
                                               size_t capacity_hint,
                                               float load_factor)
{
    return make_hashtable_with_capacity_factor_function(comparitor, capacity_hint, load_factor, identity_xor_hash);
}

Hashtable *make_hashtable_with_capacity_factor_function(compare_function comparitor,
                                                        size_t capacity_hint,
                                                        float load_factor,
                                                        hash_function function)
{
    if(NULL == comparitor || 0 == capacity_hint || 1.0f < load_factor || NULL == function)
    {
        errno = EINVAL;
        return NULL;
    }

    size_t capacity = normalize_capacity(capacity_hint);
    Hashtable *result = alloc(capacity);
    if(NULL == result)
    {
        return NULL;
    }
    init(result, comparitor, capacity, load_factor, function);

    return result;
}

static inline size_t normalize_capacity(size_t hint)
{
    if(DEFAULT_CAPACITY > hint)
    {
        return DEFAULT_CAPACITY;
    }
    size_t capacity = hint;
    if(0 != (capacity & (capacity - 1)))
    {
        // ensure that capacity is a power of 2
        capacity = 1ULL << (size_t)(log2((float)(capacity - 1)) + 1);
    }
    return capacity;
}

static Hashtable *alloc(size_t capacity)
{
    Hashtable *result = (Hashtable *)calloc(1, sizeof(Hashtable));
    if(NULL == result)
    {
        return NULL;
    }

    alloc_table(result, capacity);
    if(NULL == result->entries)
    {
        free(result);
        return NULL;
    }

    return result;
}

static inline void alloc_table(Hashtable *hashtable, size_t capacity)
{
    // the number of table cells allocated is 2x capacity to hold both keys and values
    hashtable->entries = calloc(capacity << 1, sizeof(uint8_t *));
}

static void init(Hashtable *hashtable,
                 compare_function comparitor,
                 size_t capacity,
                 float load_factor,
                 hash_function function)
{
    hashtable->occupied = 0ul;
    hashtable->capacity = (size_t)lround((float)capacity * load_factor);
    hashtable->load_factor = load_factor;
    hashtable->mutable = true;
    hashtable->length = capacity << 1;
    hashtable->hash = function;
    hashtable->compare = comparitor;
}

void hashtable_free(Hashtable *hashtable)
{
    if(NULL == hashtable)
    {
        return;
    }

    for(size_t i = 0; i < hashtable->length; i += 2)
    {
        if(CHAINED_KEY == hashtable->entries[i])
        {
            Chain *chain = (Chain *)hashtable->entries[i + 1];
            free(chain);
            hashtable->entries[i] = NULL;
            hashtable->entries[i + 1] = NULL;
        }
    }
    free(hashtable->entries);
    hashtable->entries = NULL;
    free(hashtable);
}

bool hashtable_is_mutable(const Hashtable *hashtable)
{
    return hashtable->mutable;
}

void hashtable_set_mutable(Hashtable *hashtable)
{
    hashtable->mutable = true;
}

void hashtable_set_immutable(Hashtable *hashtable)
{
    hashtable->mutable = false;
}

bool hashtable_is_immutable(const Hashtable *hashtable)
{
    return !hashtable->mutable;
}

float hashtable_load_factor(const Hashtable *hashtable)
{
    return hashtable->load_factor;
}

size_t hashtable_size(const Hashtable *hashtable)
{
    return hashtable->occupied;
}

size_t hashtable_capacity(const Hashtable *hashtable)
{
    return hashtable->capacity;
}

bool hashtable_is_empty(const Hashtable *hashtable)
{
    return 0 == hashtable->occupied;
}

void hashtable_clear(Hashtable *hashtable)
{
    if(NULL == hashtable || 0 == hashtable->occupied)
    {
        return;
    }

    for(size_t i = 0; i < hashtable->length; i += 2)
    {
        if(CHAINED_KEY == hashtable->entries[i])
        {
            Chain *chain = (Chain *)hashtable->entries[i + 1];
            free(chain);
        }
        hashtable->entries[i] = NULL;
        hashtable->entries[i + 1] = NULL;
    }
    hashtable->occupied = 0;
}

static bool contains_key_value(void *key, void *value, void *context)
{
    equality_adapter *adapter = (equality_adapter *)context;
    void *other_value = hashtable_get(adapter->other, key);
    if(NULL == other_value)
    {
        return false;
    }
    return adapter->compare(value, other_value);
}

bool hashtable_equals(const Hashtable *one, const Hashtable *two, compare_function comparitor)
{
    if(NULL == one || NULL == two || NULL == comparitor)
    {
        errno = EINVAL;
        return false;
    }

    if(one == two)
    {
        return true;
    }
    if(0 == one->occupied && 0 == two->occupied)
    {
        return true;
    }
    if(one->occupied != two->occupied)
    {
        return false;
    }
    return hashtable_iterate(one, contains_key_value, &(equality_adapter){two, comparitor});
}

bool hashtable_contains(const Hashtable *hashtable, const void *key)
{
    if(NULL == hashtable || NULL == key)
    {
        errno = EINVAL;
        return false;
    }

    if(0 == hashtable->occupied)
    {
        return false;
    }

    size_t index = hash_index(hashtable, key);
    uint8_t *cur = hashtable->entries[index];
    if(NULL != cur)
    {
        if(CHAINED_KEY != cur && hashtable->compare(key, cur))
        {
            return true;
        }
        if(CHAINED_KEY == cur)
        {
            return chained_contains(hashtable, index, key);
        }
    }

    return false;
}

static bool chained_contains(const Hashtable *hashtable, size_t index, const void *key)
{
    Chain *chain = (Chain *)hashtable->entries[index + 1];
    for(size_t i = 0; i < chain->length; i += 2)
    {
        if(NULL == chain->entries[i])
        {
            return false;
        }
        if(hashtable->compare(chain->entries[i], key))
        {
            return true;
        }
    }
    return false;
}

void *hashtable_get(const Hashtable *hashtable, const void *key)
{
    if(NULL == hashtable || NULL == key)
    {
        errno = EINVAL;
        return NULL;
    }

    if(0 == hashtable->occupied)
    {
        return NULL;
    }

    size_t index = hash_index(hashtable, key);
    uint8_t *cur = hashtable->entries[index];
    if(NULL != cur)
    {
        if(CHAINED_KEY != cur && hashtable->compare(key, cur))
        {
            void *result = hashtable->entries[index + 1];
            return result;
        }
        if(CHAINED_KEY == cur)
        {
            return chained_get(hashtable, index, key);
        }
    }

    return NULL;
}

static void *chained_get(const Hashtable *hashtable, size_t index, const void *key)
{
    Chain *chain = (Chain *)hashtable->entries[index + 1];
    for(size_t i = 0; i < chain->length; i += 2)
    {
        if(NULL == chain->entries[i])
        {
            return NULL;
        }
        if(hashtable->compare(chain->entries[i], key))
        {
            void *result = chain->entries[i + 1];
            if(0 != i)
            {
                // N.B. - move most recent fetch to front
                uint8_t *tmp = chain->entries[0];
                chain->entries[0] = chain->entries[i];
                chain->entries[i] = tmp;
                tmp = chain->entries[1];
                chain->entries[1] = chain->entries[i + 1];
                chain->entries[i + 1] = tmp;
            }
            return result;
        }
    }
    return NULL;
}

void *hashtable_get_if_absent(Hashtable *hashtable, void *key, void *value)
{
    if(NULL == hashtable || NULL == key || NULL == value)
    {
        errno = EINVAL;
        return NULL;
    }

    if(0 == hashtable->occupied)
    {
        return value;
    }

    errno = 0;
    void *result = hashtable_get(hashtable, key);
    if(0 != errno)
    {
        return NULL;
    }
    if(NULL == result && 0 == errno)
    {
        return value;
    }
    return result;
}

void *hashtable_get_if_absent_put(Hashtable *hashtable, void *key, void *value)
{
    if(NULL == hashtable || NULL == key || NULL == value)
    {
        errno = EINVAL;
        return NULL;
    }

    if(0 == hashtable->occupied)
    {
        hashtable_put(hashtable, key, value);
        if(0 != errno)
        {
            return NULL;
        }
        return value;
    }

    errno = 0;
    void *result = hashtable_get(hashtable, key);
    if(0 != errno)
    {
        return NULL;
    }
    if(NULL == result && 0 == errno)
    {
        hashtable_put(hashtable, key, value);
        if(0 != errno)
        {
            return NULL;
        }
        return value;
    }
    return result;
}

void *hashtable_put(Hashtable *hashtable, void *key, void *value)
{
    if(NULL == hashtable || hashtable_is_immutable(hashtable) || NULL == key || NULL == value)
    {
        errno = EINVAL;
        return NULL;
    }

    size_t index = hash_index(hashtable, key);
    uint8_t *cur = hashtable->entries[index];
    if(NULL == cur)
    {
        hashtable->entries[index] = key;
        hashtable->entries[index + 1] = value;
        if(++hashtable->occupied > hashtable->capacity)
        {
            rehash(hashtable);
        }
        return NULL;
    }
    if(CHAINED_KEY != cur && hashtable->compare(key, cur))
    {
        void *previous = hashtable->entries[index + 1];
        hashtable->entries[index + 1] = value;
        return previous;
    }
    if(CHAINED_KEY == hashtable->entries[index])
    {
        return add_to_chain(hashtable, index, key, value);
    }
    return chained_put(hashtable, index, key, value);
}

static void *add_to_chain(Hashtable *hashtable, size_t index, void *key, void *value)
{
    Chain *chain = (Chain *)hashtable->entries[index + 1];
    for(size_t i = 0; i < chain->length; i += 2)
    {
        if(NULL == chain->entries[i])
        {
            chain->entries[i] = chain->entries[0];
            chain->entries[i + 1] = chain->entries[1];
            chain->entries[0] = key;
            chain->entries[1] = value;
            if(++hashtable->occupied > hashtable->capacity)
            {
                rehash(hashtable);
            }
            return NULL;
        }
        if(hashtable->compare(chain->entries[i], key))
        {
            void *previous = chain->entries[i + 1];
            uint8_t *tmp = chain->entries[0];
            chain->entries[0] = chain->entries[i];
            chain->entries[i] = tmp;
            chain->entries[i + 1] = chain->entries[1];
            chain->entries[1] = value;
            return previous;
        }
    }
    expand_chain(hashtable, index, key, value);
    return NULL;
}

static void expand_chain(Hashtable *hashtable, size_t index, void *key, void *value)
{
    Chain *chain = (Chain *)hashtable->entries[index + 1];
    Chain *expansion = calloc(1, sizeof(Chain) + (sizeof(uint8_t *) * (chain->length + 2)));
    if(NULL == expansion)
    {
        return;
    }
    expansion->length = chain->length + 2;
    memcpy(expansion->entries + 2, chain->entries, sizeof(uint8_t *) * chain->length);
    expansion->entries[0] = key;
    expansion->entries[1] = value;
    hashtable->entries[index + 1] = (uint8_t *)expansion;
    free(chain);
    if(++hashtable->occupied > hashtable->capacity)
    {
        rehash(hashtable);
    }
}

static void *chained_put(Hashtable *hashtable, size_t index, void *key, void *value)
{
    Chain *chain = malloc(sizeof(Chain) + sizeof(uint8_t *) * 4);
    if(NULL == chain)
    {
        return NULL;
    }
    chain->length = 4;
    // N.B. - last in is moved to the front
    chain->entries[0] = key;
    chain->entries[1] = value;
    chain->entries[2] = hashtable->entries[index];
    chain->entries[3] = hashtable->entries[index + 1];
    hashtable->entries[index] = (uint8_t *)CHAINED_KEY;
    hashtable->entries[index + 1] = (uint8_t *)chain;
    if(++hashtable->occupied > hashtable->capacity)
    {
        rehash(hashtable);
    }
    return NULL;
}

static bool map_into(void *key, void *value, void *context)
{
    Hashtable *hashtable = (Hashtable *)context;
    errno = 0;
    hashtable_put(hashtable, key, value);
    return 0 == errno;
}

void hashtable_put_all(Hashtable *to, const Hashtable *from)
{
    if(NULL == to || NULL == from)
    {
        errno = EINVAL;
        return;
    }

    if(0 == from->occupied)
    {
        return;
    }

    hashtable_iterate(from, map_into, to);
}

Hashtable *hashtable_copy(const Hashtable *hashtable)
{
    if(NULL == hashtable)
    {
        errno = EINVAL;
        return NULL;
    }

    Hashtable *result = make_hashtable_with_capacity_factor_function(
        hashtable->compare, hashtable->capacity, hashtable->load_factor, hashtable->hash);
    hashtable_put_all(result, hashtable);
    return result;
}

void *hashtable_remove(Hashtable *hashtable, void *key)
{
    if(NULL == hashtable || NULL == key)
    {
        errno = EINVAL;
        return NULL;
    }

    if(0 == hashtable->occupied)
    {
        return NULL;
    }

    size_t index = hash_index(hashtable, key);
    uint8_t *cur = hashtable->entries[index];
    if(NULL != cur)
    {
        if(CHAINED_KEY != cur && hashtable->compare(key, cur))
        {
            hashtable->entries[index] = NULL;
            void *previous = hashtable->entries[index + 1];
            hashtable->entries[index + 1] = NULL;
            hashtable->occupied--;
            return previous;
        }
        if(CHAINED_KEY == cur)
        {
            return chained_remove(hashtable, index, key);
        }
    }

    return NULL;
}

static void *chained_remove(Hashtable *hashtable, size_t index, void *key)
{
    Chain *chain = (Chain *)hashtable->entries[index + 1];
    for(size_t i = 0; i < chain->length; i += 2)
    {
        if(NULL == chain->entries[i])
        {
            return NULL;
        }
        if(hashtable->compare(chain->entries[i], key))
        {
            void *previous = chain->entries[i + 1];
            if(chain->length - 2 == i)
            {
                chain->entries[i] = NULL;
                chain->entries[i + 1] = NULL;
            }
            else
            {
                // N.B. - preserve any move-to-front ordering
                memcpy(chain->entries + i, chain->entries + i + 2, sizeof(uint8_t *) * (chain->length - (i + 2)));
                chain->entries[chain->length - 1] = NULL;
                chain->entries[chain->length - 2] = NULL;
            }
            if(NULL == chain->entries[0])
            {
                // N.B. - empty chains can be removed and the bucket can be freed
                hashtable->entries[index] = NULL;
                hashtable->entries[index + 1] = NULL;
                free(chain);
            }
            else if(NULL == chain->entries[2])
            {
                // N.B. - chains with only one entry can be collapsed into a bucket
                hashtable->entries[index] = chain->entries[0];
                hashtable->entries[index + 1] = chain->entries[1];
                free(chain);
            }
            hashtable->occupied--;
            return previous;
        }
    }
    return NULL;
}

bool hashtable_iterate(const Hashtable *hashtable, hashtable_iterator iterator, void *context)
{
    if(NULL == hashtable || NULL == iterator)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < hashtable->length; i += 2)
    {
        if(CHAINED_KEY == hashtable->entries[i])
        {
            Chain *chain = (Chain *)hashtable->entries[i + 1];
            for(size_t j = 0; j < chain->length && NULL != chain->entries[j]; j += 2)
            {
                if(!iterator(chain->entries[j], chain->entries[j + 1], context))
                {
                    return false;
                }
            }
        }
        else if(NULL != hashtable->entries[i])
        {
            if(!iterator(hashtable->entries[i], hashtable->entries[i + 1], context))
            {
                return false;
            }
        }
    }

    return true;
}

static bool key_item_iterator(void *key, void *value __attribute__((unused)), void *context)
{
    item_adapter *adapter = (item_adapter *)context;
    return adapter->iterator(key, adapter->original_context);
}

static bool value_item_iterator(void *key __attribute__((unused)), void *value, void *context)
{
    item_adapter *adapter = (item_adapter *)context;
    return adapter->iterator(value, adapter->original_context);
}

bool hashtable_iterate_keys(const Hashtable *hashtable, hashtable_item_iterator iterator, void *context)
{
    return hashtable_iterate(hashtable, key_item_iterator, &(item_adapter){iterator, context});
}

bool hashtable_iterate_values(const Hashtable *hashtable, hashtable_item_iterator iterator, void *context)
{
    return hashtable_iterate(hashtable, value_item_iterator, &(item_adapter){iterator, context});
}

static inline size_t hash_index(const Hashtable *hashtable, const void * key)
{
    hashcode h = hashtable->hash(key);
    return (h & ((hashtable->length >> 1) - 1)) << 1;
}

static void rehash(Hashtable *hashtable)
{
    size_t capacity = normalize_capacity((hashtable->length >> 1) + 1);
    uint8_t **table = hashtable->entries;
    size_t length = hashtable->length;
    alloc_table(hashtable, capacity);
    init(hashtable, hashtable->compare, capacity, hashtable->load_factor, hashtable->hash);
    for(size_t i = 0; i < length; i += 2)
    {
        if(CHAINED_KEY == table[i])
        {
            Chain *chain = (Chain *)table[i + 1];
            for(size_t j = 0; j < chain->length; j += 2)
            {
                if(NULL == chain->entries[j])
                {
                    break;
                }
                hashtable_put(hashtable, chain->entries[j], chain->entries[j + 1]);
            }
            free(chain);
            table[i] = NULL;
            table[i + 1] = NULL;
        }
        else if(NULL != table[i])
        {
            hashtable_put(hashtable, table[i], table[i + 1]);
        }
    }
    free(table);
}

void hashtable_summary(const Hashtable *hashtable, FILE *stream)
{
    fprintf(stream, "hashtable summary:\n");
    fprintf(stream, "mutable: %s\n", hashtable_is_mutable(hashtable) ? "yes" : "no");
    fprintf(stream, "occupied: %zd of %zd\n", hashtable->occupied, hashtable->capacity);
    fprintf(stream, "capacity: %zd (%zd * %g)\n", hashtable->capacity, hashtable->length >> 1, hashtable->load_factor);
    fprintf(stream, "table length: %zd\n", hashtable->length);
    fprintf(stream, "load factor: %g\n", hashtable->load_factor);
    fprintf(stream, "bucket report:\n");
    size_t count = 0ul, min = 0ul, max = 0ul, total = 0ul;
    float avg = 0.0f;
    for(size_t i = 0; i < hashtable->length; i += 2)
    {
        if(CHAINED_KEY == hashtable->entries[i])
        {
            count++;
            Chain *chain = (Chain *)hashtable->entries[i + 1];
            fprintf(stream, "[%zd]: chain (length: %zd, hash: 0x%zx)\n", i, chain->length, hashtable->hash(chain->entries[0]));
            for(size_t j = 0; j < chain->length && NULL != chain->entries[j]; j += 2)
            {
                fprintf(stream, "  [%zd]: chained key: \"%s\"\n", j, chain->entries[j]);
            }
            if(max < chain->length)
            {
                max = chain->length;
            }
            if(0 == min || min > chain->length)
            {
                min = chain->length;
            }
            total += chain->length;
            avg = (float)total / (float)count;
        }
        else if(NULL != hashtable->entries[i])
        {
            fprintf(stream, "[%zd]: key: \"%s\" hash: 0x%zx\n", i, hashtable->entries[i], hashtable->hash(hashtable->entries[i]));
        }
    }
    fprintf(stream, "chains: %zd (length min: %zd, max: %zd, avg: %g)\n", count, min, max, avg);
}
