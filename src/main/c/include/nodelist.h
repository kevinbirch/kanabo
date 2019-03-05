#pragma once

#include "document.h"
#include "vector.h"

typedef Vector Nodelist;

/*
 * Constructors
 */

#define      make_nodelist make_vector
#define      make_nodelist_with_capacity make_vector_with_capacity
#define      make_nodelist_of make_vector_of

/*
 * Destructors
 */

#define      dispose_nodelist dispose_vector
void         nodelist_destroy(Nodelist *self);

/*
 * Property Access
 */

#define      nodelist_length vector_length
#define      nodelist_is_empty vector_is_empty

/*
 * Element Access
 */

#define      nodelist_get vector_get
#define      nodelist_add vector_add
#define      nodelist_set vector_set

/*
 * Iteration
 */

typedef bool (*nodelist_iterator)(Node *each, void *context);
bool         nodelist_iterate(const Nodelist *self, nodelist_iterator iterator, void *context);

typedef bool (*nodelist_map_function)(Node *each, void *context, Nodelist *target);

Nodelist    *nodelist_map(const Nodelist *self, nodelist_map_function function, void *context);
