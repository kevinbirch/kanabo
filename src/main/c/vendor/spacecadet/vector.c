#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "vector.h"
#include "xalloc.h"

static const size_t DEFAULT_CAPACITY = 4;

struct vector_s
{
    size_t    length;
    size_t    capacity;
    uint8_t **items;
};

static inline size_t calculate_new_capacity(size_t capacity)
{
    return (capacity * 3) / 2 + 1;
}

static inline void reallocate(Vector *vector, size_t capacity)
{
    vector->items = xrealloc(vector->items, sizeof(uint8_t *) * capacity);
    vector->capacity = capacity;
}

static inline void ensure_capacity(Vector *vector, size_t min_capacity)
{
    if(vector->capacity < min_capacity)
    {
        size_t new_capacity = calculate_new_capacity(min_capacity);
        reallocate(vector, new_capacity);
    }
}

Vector *make_vector(void)
{
    return make_vector_with_capacity(DEFAULT_CAPACITY);
}

Vector *make_vector_with_capacity(size_t capacity)
{
    if(0 == capacity)
    {
        errno = EINVAL;
        return NULL;
    }

    Vector *result = (Vector *)xcalloc(sizeof(Vector));
    result->items = (uint8_t **)xcalloc(sizeof(uint8_t *) * capacity);
    result->length = 0;
    result->capacity = capacity;

    return result;
}

Vector *make_vector_of(size_t count, ...)
{
    if(0 == count)
    {
        errno = EINVAL;
        return NULL;
    }

    Vector *result = make_vector_with_capacity(count);

    va_list rest;
    va_start(rest, count);
    for(size_t i = 0; i < count; i++)
    {
        void *element = va_arg(rest, void *);
        result->items[i] = element;
    }
    va_end(rest);
    result->length = count;

    return result;
}

Vector *vector_copy(const Vector *vector)
{
    if(NULL == vector)
    {
        errno = EINVAL;
        return NULL;
    }

    if(0 == vector->length)
    {
        return make_vector_with_capacity(1);
    }

    Vector *result = make_vector_with_capacity(vector->length);
    memcpy(result->items, vector->items, sizeof(uint8_t *) * vector->length);
    result->length = vector->length;

    return result;
}

Vector *vector_with(const Vector *vector, void *value)
{
    if(NULL == vector || NULL == value)
    {
        errno = EINVAL;
        return NULL;
    }

    Vector *result = vector_copy(vector);
    vector_add(result, value);

    return result;
}

Vector *vector_with_all(const Vector *vector, const Vector *from)
{
    if(NULL == vector || NULL == from)
    {
        errno = EINVAL;
        return NULL;
    }

    Vector *result = vector_copy(vector);
    vector_add_all(result, from);

    return result;
}

void dispose_vector(Vector *vector)
{
    if(NULL == vector)
    {
        return;
    }

    if(NULL != vector->items)
    {
        free(vector->items);
        vector->items = NULL;
    }
    free(vector);
}

void vector_destroy(Vector *vector, vector_item_destructor destructor)
{
    if(NULL == vector)
    {
        return;
    }
    if(NULL == destructor)
    {
        errno = EINVAL;
        return;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        destructor(vector->items[i]);
    }

    dispose_vector(vector);
}

size_t vector_length(const Vector *vector)
{
    if(NULL == vector)
    {
        errno = EINVAL;
        return 0;
    }

    return vector->length;
}

size_t vector_capacity(const Vector *vector)
{
    if(NULL == vector)
    {
        errno = EINVAL;
        return 0;
    }

    return vector->capacity;
}

bool vector_is_empty(const Vector *vector)
{
    if(NULL == vector)
    {
        errno = EINVAL;
        return true;
    }

    return 0 == vector->length;
}

void *vector_get(const Vector *vector, size_t index)
{
    if(NULL == vector || index >= vector->length)
    {
        errno = EINVAL;
        return NULL;
    }

    return vector->items[index];
}

void *vector_first(const Vector *vector)
{
    return vector_get(vector, 0);
}

void *vector_last(const Vector *vector)
{
    if(NULL == vector || 0 == vector->length)
    {
        errno = EINVAL;
        return NULL;
    }

    return vector_get(vector, vector->length - 1);
}

void vector_add(Vector *vector, void *value)
{
    if(NULL == vector || NULL == value)
    {
        errno = EINVAL;
        return;
    }

    ensure_capacity(vector, vector->length + 1);
    vector->items[vector->length++] = (void *)value;
}

static bool add_to_vector_iterator(void *each, void *context)
{
    Vector *vector = (Vector *)context;

    vector_add(vector, each);
    return true;
}

bool vector_add_all(Vector *vector, const Vector *from)
{
    if(NULL == vector || NULL == from)
    {
        errno = EINVAL;
        return false;
    }

    ensure_capacity(vector, vector->length + from->length);
    return vector_iterate(from, add_to_vector_iterator, vector);
}

void vector_insert(Vector *vector, void *value, size_t index)
{
    if(NULL == vector || NULL == value || index > vector->length)
    {
        errno = EINVAL;
        return;
    }

    if(vector->length == index)
    {
        vector_add(vector, value);
    }

    uint8_t **target = vector->items;
    if(vector->capacity < vector->length + 1)
    {
        size_t new_capacity = calculate_new_capacity(vector->capacity);
        target = xcalloc(sizeof(uint8_t *) * new_capacity);
        vector->capacity = new_capacity;
    }

    if(0 == index)
    {
        memmove(target + 1, vector->items, sizeof(uint8_t *) * vector->length);
        target[0] = value;
    }
    else
    {
        if(target != vector->items)
        {
            memcpy(target, vector->items, sizeof(uint8_t *) * index);
        }
        memmove(target + index + 1, vector->items + index,
                sizeof(uint8_t *) * (vector->length - index));
        target[index] = value;
    }

    if(target != vector->items)
    {
        free(vector->items);
        vector->items = target;
    }
    vector->length++;
}

void *vector_set(Vector *vector, void *value, size_t index)
{
    if(NULL == vector || NULL == value || index >= vector->length)
    {
        errno = EINVAL;
        return NULL;
    }

    void *previous = vector->items[index];
    vector->items[index] = value;
    return previous;
}

void *vector_remove(Vector *vector, size_t index)
{
    if(NULL == vector || index >= vector->length)
    {
        errno = EINVAL;
        return NULL;
    }

    void *previous = vector->items[index];
    if(index == vector->length - 1)
    {
        vector->items[index] = NULL;
    }
    else
    {
        memmove(vector->items + index, vector->items + index + 1, sizeof(uint8_t *) * (vector->length - (index + 1)));
        vector->items[vector->length - 1] = NULL;
    }
    vector->length--;

    return previous;
}

bool vector_remove_item(Vector *vector, vector_comparitor comparitor, void *item)
{
    if(NULL == vector || NULL == comparitor || NULL == item)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(comparitor(vector->items[i], item))
        {
            return NULL != vector_remove(vector, i);
        }
    }

    return false;
}

void vector_clear(Vector *vector)
{
    if(NULL == vector || 0 == vector->length)
    {
        return;
    }

    memset(vector->items, 0, sizeof(uint8_t *) * vector->length);
    vector->length = 0;
}

bool vector_trim(Vector *vector)
{
    if(NULL == vector || 0 == vector->length)
    {
        errno = EINVAL;
        return false;
    }

    if(vector->length == vector->capacity)
    {
        return false;
    }

    reallocate(vector, vector->length);
    return true;
}

void *vector_find(const Vector *vector, vector_iterator iterator, void *context)
{
    if(NULL == vector || NULL == iterator)
    {
        errno = EINVAL;
        return NULL;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(iterator(vector->items[i], context))
        {
            return vector->items[i];
        }
    }

    return NULL;
}

bool vector_contains(const Vector *vector, vector_comparitor comparitor, void *item)
{
    if(NULL == vector || NULL == comparitor || NULL == item)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(comparitor(vector->items[i], item))
        {
            return true;
        }
    }

    return false;
}

bool vector_any(const Vector *vector, vector_iterator iterator, void *context)
{
    if(NULL == vector || NULL == iterator)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(iterator(vector->items[i], context))
        {
            return true;
        }
    }

    return false;
}

bool vector_all(const Vector *vector, vector_iterator iterator, void *context)
{
    if(NULL == vector || NULL == iterator)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(!iterator(vector->items[i], context))
        {
            return false;
        }
    }

    return true;
}

bool vector_none(const Vector *vector, vector_iterator iterator, void *context)
{
    if(NULL == vector || NULL == iterator)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(iterator(vector->items[i], context))
        {
            return false;
        }
    }

    return true;
}

size_t vector_count(const Vector *vector, vector_iterator iterator, void *context)
{
    if(NULL == vector || NULL == iterator)
    {
        errno = EINVAL;
        return 0;
    }

    size_t count = 0;
    for(size_t i = 0; i < vector->length; i++)
    {
        if(iterator(vector->items[i], context))
        {
            count++;
        }
    }

    return count;
}

bool vector_equals(const Vector *one, const Vector *two, vector_comparitor comparitor)
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
    if(0 == one->length && 0 == two->length)
    {
        return true;
    }
    if(one->length != two->length)
    {
        return false;
    }
    for(size_t i = 0; i < vector_length(one); i++)
    {
        if(!comparitor(one->items[i], two->items[i]))
        {
            return false;
        }
    }

    return true;
}

bool vector_iterate(const Vector *vector, vector_iterator iterator, void *context)
{
    if(NULL == vector || NULL == iterator)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(!iterator(vector->items[i], context))
        {
            return false;
        }
    }

    return true;
}

Vector *vector_map(const Vector *vector, vector_mapper fn, void *context)
{
    if(NULL == vector || NULL == fn)
    {
        errno = EINVAL;
        return false;
    }

    if(0 == vector->length)
    {
        return make_vector_with_capacity(1);
    }

    Vector *target = make_vector_with_capacity(vector->length);
    vector_map_into(vector, fn, context, target);

    return target;
}

bool vector_map_into(const Vector *vector, vector_mapper fn, void *context, Vector *target)
{
    if(NULL == vector || NULL == fn || NULL == target)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector->length; i++)
    {
        if(!fn(vector->items[i], context, target))
        {
            return false;
        }
    }

    return true;
}

void *vector_reduce(const Vector *vector, vector_reducer fn, void *context)
{
    if(NULL == vector || NULL == fn || 0 == vector->length)
    {
        errno = EINVAL;
        return NULL;
    }

    if(1 == vector->length)
    {
        return vector->items[0];
    }

    void *result = fn(vector->items[0], vector->items[1], context);
    for(size_t i = 2; i < vector->length; i++)
    {
        result = fn(result, vector->items[i], context);
    }

    return result;
}

Vector *vector_filter(const Vector *vector, vector_iterator fn, void *context)
{
    if(NULL == vector || NULL == fn)
    {
        errno = EINVAL;
        return NULL;
    }

    if(0 == vector->length)
    {
        return make_vector_with_capacity(1);
    }

    Vector *result = make_vector_with_capacity(vector->length);
    for(size_t i = 0; i < vector->length; i++)
    {
        if(fn(vector->items[i], context))
        {
            result->items[result->length++] = vector->items[i];
        }
    }

    return result;
}

Vector *vector_filter_not(const Vector *vector, vector_iterator fn, void *context)
{
    if(NULL == vector || NULL == fn)
    {
        errno = EINVAL;
        return NULL;
    }

    if(0 == vector->length)
    {
        return make_vector_with_capacity(1);
    }

    Vector *result = make_vector_with_capacity(vector->length);
    for(size_t i = 0; i < vector->length; i++)
    {
        if(!fn(vector->items[i], context))
        {
            result->items[result->length++] = vector->items[i];
        }
    }

    return result;
}

void vector_dump(const Vector *vector, FILE *stream)
{
    fprintf(stream,
            "vector summary:\n"
            "length: %zu\n"
            "capacity: %zu\n",
            vector->length, vector->capacity);
    for(size_t i = 0; i < vector->length; i++)
    {
        fprintf(stream, "[%zu]: (%p)\n", i, (void *)vector->items[i]);
    }
}
