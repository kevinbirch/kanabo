#pragma once

#include "document.h"
#include "vector.h"

typedef Vector Nodelist;

/*
 * Constructors
 */

#define      make_nodelist make_vector
#define      make_nodelist_of make_vector_of

/*
 * Destructors
 */

#define      dispose_nodelist dispose_vector

/*
 * Property Access
 */

#define      nodelist_length vector_length
#define      nodelist_is_empty vector_is_empty

/*
 * Element Access
 */

#define      nodelist_get vector_get
void         nodelist_add(Nodelist *list, Node *value);
void         nodelist_set(Nodelist *list, Node *value, size_t index);

/*
 * Iteration
 */

typedef bool (*nodelist_iterator)(Node *each, void *context);
bool         nodelist_iterate(const Nodelist *list, nodelist_iterator iterator, void *context);

typedef bool (*nodelist_map_function)(Node *each, void *context, Nodelist *target);

Nodelist    *nodelist_map(const Nodelist *list, nodelist_map_function function, void *context);
Nodelist    *nodelist_map_into(const Nodelist *list, nodelist_map_function function, void *context, Nodelist *target);
