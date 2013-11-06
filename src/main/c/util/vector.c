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

#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "vector.h"

static const size_t DEFAULT_CAPACITY = 4;

struct vector_s
{
    size_t    length;
    size_t    capacity;
    uint8_t **items;
};

static inline bool ensure_capacity(Vector *vector, size_t min_capacity);

static bool add_to_vector_iterator(void *each, void *context);

Vector *make_vector(void)
{
    return make_vector_with_capacity(DEFAULT_CAPACITY);
}

Vector *make_vector_with_capacity(size_t capacity)
{
    Vector *result = (Vector *)calloc(1, sizeof(Vector));
    if(NULL == result)
    {
        return NULL;
    }

    result->items = (uint8_t **)calloc(1, sizeof(uint8_t *) * capacity);
    if(NULL == result->items)
    {
        free(result);
        return NULL;
    }

    result->length = 0;
    result->capacity = capacity;
    return result;
}

void vector_free(Vector *vector)
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

bool vector_add(Vector *vector, void *value)
{
    if(NULL == vector || NULL == value)
    {
        errno = EINVAL;
        return false;
    }

    if(!ensure_capacity(vector, vector->length + 1))
    {
        return false;
    }
    vector->items[vector->length++] = value;
    return true;
}

static bool add_to_vector_iterator(void *each, void *context)
{
    Vector *vector = (Vector *)context;
    return vector_add(vector, each);
}

bool vector_add_all(Vector *vector, Vector *value)
{
    if(NULL == vector || NULL == value)
    {
        errno = EINVAL;
        return false;
    }

    if(!ensure_capacity(vector, vector->length + value->length))
    {
        return false;
    }
    bool result = vector_iterate(value, add_to_vector_iterator, vector);
    return result;
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
        memcpy(vector->items + index, vector->items + index + 1, sizeof(uint8_t *) * (vector->length - (index + 1)));
        vector->items[vector->length - 1] = NULL;
    }
    vector->length--;
    return previous;
}

void vector_clear(Vector *vector)
{
    if(NULL == vector)
    {
        return;
    }

    if(0 != vector->length)
    {
        memset(vector->items, 0, sizeof(uint8_t *) * vector->length);
        vector->length = 0;
    }
}

bool vector_trim(Vector *vector)
{
    if(vector->length == vector->capacity)
    {
        return false;
    }

    uint8_t **cache = vector->items;
    vector->items = realloc(vector->items, sizeof(void *) * vector->length);
    if(NULL == vector->items)
    {
        vector->items = cache;
        return false;
    }
    vector->capacity = vector->length;
    return true;
}

bool vector_equals(const Vector *one, const Vector *two, vector_item_comparitor comparitor)
{
    if(one == two)
    {
        return true;
    }
    if(vector_length(one) != vector_length(two))
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

    for(size_t i = 0; i < vector_length(vector); i++)
    {
        if(!iterator(vector->items[i], context))
        {
            return false;
        }
    }
    return true;
}

Vector *vector_map(const Vector *vector, vector_map_function function, void *context)
{
    if(NULL == vector || NULL == function)
    {
        errno = EINVAL;
        return false;
    }

    Vector *target = make_vector_with_capacity(vector_length(vector));
    if(NULL == target)
    {
        return NULL;
    }
    Vector *result = vector_map_into(vector, function, context, target);
    if(NULL == result)
    {
        vector_free(target);
        target = NULL;
        return NULL;
    }
    return target;            
}

Vector *vector_map_into(const Vector *vector, vector_map_function function, void *context, Vector *target)
{
    if(NULL == vector || NULL == function || NULL == target)
    {
        errno = EINVAL;
        return false;
    }

    for(size_t i = 0; i < vector_length(vector); i++)
    {
        if(!function(vector->items[i], context, target))
        {
            return NULL;
        }
    }

    return target;
}

static inline bool ensure_capacity(Vector *vector, size_t min_capacity)
{
    if(vector->capacity < min_capacity)
    {
        size_t new_capacity = (min_capacity * 3) / 2 + 1;
        uint8_t **cache = vector->items;
        vector->items = realloc(vector->items, sizeof(void *) * new_capacity);
        if(NULL == vector->items)
        {
            vector->items = cache;
            return false;
        }
        vector->capacity = new_capacity;
    }
    return true;
}


