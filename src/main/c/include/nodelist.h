#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include "document.h"
#include "vector.h"

typedef Vector nodelist;

#define make_nodelist make_vector
#define make_nodelist_of make_vector_of
#define nodelist_free vector_free

#define nodelist_length   vector_length
#define nodelist_is_empty vector_is_empty

#define nodelist_get    vector_get
#define nodelist_add    vector_add
bool nodelist_set(nodelist *list, void *value, size_t index);

typedef bool (*nodelist_iterator)(Node *each, void *context);
bool nodelist_iterate(const nodelist *list, nodelist_iterator iterator, void *context);

typedef bool (*nodelist_map_function)(Node *each, void *context, nodelist *target);

nodelist *nodelist_map(const nodelist *list, nodelist_map_function function, void *context);
nodelist *nodelist_map_into(const nodelist *list, nodelist_map_function function, void *context, nodelist *target);
