#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "hash.h"

typedef struct hashtable_s Hashtable;

typedef bool (*hashtable_iterator)(void *key, void *value, void *context);
typedef bool (*hashtable_item_iterator)(void *item, void *context);

Hashtable *make_hashtable(compare_function comparitor);
Hashtable *make_hashtable_with_function(compare_function comparitor,
                                        hash_function function);
Hashtable *make_hashtable_with_capacity(compare_function comparitor,
                                        size_t capacity_hint);
Hashtable *make_hashtable_with_capacity_function(compare_function comparitor,
                                                 size_t capacity_hint,
                                                 hash_function function);
Hashtable *make_hashtable_with_capacity_factor(compare_function comparitor,
                                               size_t capacity_hint,
                                               float load_factor);
Hashtable *make_hashtable_with_capacity_factor_function(compare_function comparitor,
                                                        size_t capacity_hint, 
                                                        float load_factor, 
                                                        hash_function function);

void hashtable_free(Hashtable *hashtable);

size_t hashtable_size(const Hashtable *hashtable);
size_t hashtable_capacity(const Hashtable *hashtable);
bool hashtable_is_empty(const Hashtable *hashtable);
float hashtable_load_factor(const Hashtable *hashtable);

bool hashtable_is_mutable(const Hashtable *hashtable);
void hashtable_set_mutable(Hashtable *hashtable);

bool hashtable_is_immutable(const Hashtable *hashtable);
void hashtable_set_immutable(Hashtable *hashtable);

bool hashtable_equals(const Hashtable *hashtable1, 
                      const Hashtable *hashtable2, 
                      compare_function comparitor);

bool hashtable_contains(const Hashtable *hashtable, const void *key);
void *hashtable_get(const Hashtable *hashtable, const void *key);
void *hashtable_get_if_absent(Hashtable *hashtable, void * key, void * value);
void *hashtable_get_if_absent_put(Hashtable *hashtable, void *key, void *value);

void *hashtable_put(Hashtable *hashtable, void *key, void *value);
void hashtable_put_all(Hashtable *to, const Hashtable * from);

Hashtable *hashtable_copy(const Hashtable * hashtable);

void *hashtable_remove(Hashtable *hashtable, void *key);
void hashtable_clear(Hashtable *hashtable);

bool hashtable_iterate(const Hashtable *hashtable, hashtable_iterator iterator, void *context);
bool hashtable_iterate_keys(const Hashtable *hashtable, hashtable_item_iterator iterator, void *context);
bool hashtable_iterate_values(const Hashtable *hashtable, hashtable_item_iterator iterator, void *context);

void hashtable_summary(const Hashtable *hashtable, FILE *stream);
