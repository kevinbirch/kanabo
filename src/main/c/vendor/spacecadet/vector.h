#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct vector_s Vector;

/* Callback Functions */
typedef bool  (*vector_iterator)(void *each, void *context);
typedef bool  (*vector_mapper)(void *each, void *context, Vector *target);
typedef bool  (*vector_comparitor)(const void *one, const void *two);
typedef void *(*vector_reducer)(const void *one, const void *two, void *context);
typedef void  (*vector_item_destructor)(void *each);

/* Constructors */
Vector *make_vector(void);
Vector *make_vector_with_capacity(size_t capacity);
Vector *make_vector_of(size_t count, ...);
Vector *vector_copy(const Vector *vector);
Vector *vector_with(const Vector *vector, void *value);
Vector *vector_with_all(const Vector *vector, const Vector *from);

/* Destructors */
void    dispose_vector(Vector *vector);
void    vector_destroy(Vector *vector, vector_item_destructor destructor);

/* Size API */
size_t  vector_length(const Vector *vector);
size_t  vector_capacity(const Vector *vector);
bool    vector_is_empty(const Vector *vector);

/* Element API */
void   *vector_get(const Vector *vector, size_t index);
void   *vector_first(const Vector *vector);
#define vector_head vector_first
void   *vector_last(const Vector *vector);

/* Mutation API */
void    vector_add(Vector *vector, void *value);
#define vector_append vector_add
bool    vector_add_all(Vector *vector, const Vector *value);
void    vector_insert(Vector *vector, void *value, size_t index);
#define vector_prepend(VECTOR, VALUE) vector_insert((VECTOR), (VALUE), 0)
void   *vector_set(Vector *vector, void *value, size_t index);

/* Deletion API */
void   *vector_remove(Vector *vector, size_t index);
bool    vector_remove_item(Vector *vector, vector_comparitor comparitor, void *item);
void    vector_clear(Vector *vector);
bool    vector_trim(Vector *vector);

/* Queue API */
#define vector_peek(VECTOR) vector_get((VECTOR), vector_length((VECTOR)) - 1)
#define vector_push vector_add
#define vector_pop(VECTOR) vector_remove((VECTOR), vector_length((VECTOR)) - 1)

/* Search API */
void   *vector_find(const Vector *vector, vector_iterator iterator, void *context);
bool    vector_contains(const Vector *vector, vector_comparitor comparitor, void *item);
bool    vector_any(const Vector *vector, vector_iterator iterator, void *context);
bool    vector_all(const Vector *vector, vector_iterator iterator, void *context);
bool    vector_none(const Vector *vector, vector_iterator iterator, void *context);
size_t  vector_count(const Vector *vector, vector_iterator iterator, void *context);

/* Comparison API */
bool    vector_equals(const Vector *one, const Vector *two, vector_comparitor comparitor);

/* Functional API */
bool    vector_iterate(const Vector *vector, vector_iterator iterator, void *context);
Vector *vector_map(const Vector *vector, vector_mapper function, void *context);
bool    vector_map_into(const Vector *vector, vector_mapper function, void *context, Vector *target);
void   *vector_reduce(const Vector *vector, vector_reducer function, void *context);
Vector *vector_filter(const Vector *vector, vector_iterator function, void *context);
Vector *vector_filter_not(const Vector *vector, vector_iterator function, void *context);

void vector_dump(const Vector *vector, FILE *stream);
