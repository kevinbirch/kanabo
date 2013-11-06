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

#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct vector_s Vector;

typedef bool (*vector_iterator)(void *each, void *context);
typedef bool (*vector_map_function)(void *each, void *context, Vector *target);
typedef bool (*vector_item_comparitor)(const void *one, const void *two);

Vector *make_vector(void);
Vector *make_vector_with_capacity(size_t capacity);

void    vector_free(Vector *value);
        
size_t  vector_length(const Vector *vector);
size_t  vector_capacity(const Vector *vector);
bool    vector_is_empty(const Vector *vector);
        
void   *vector_get(const Vector *vector, size_t index);
bool    vector_add(Vector *vector,  void *value);
bool    vector_add_all(Vector *vector,  Vector *value);
void   *vector_set(Vector *vector, void *value, size_t index);
void   *vector_remove(Vector *vector, size_t index);

void    vector_clear(Vector *vector);
bool    vector_trim(Vector *vector);
        
bool    vector_equals(const Vector *one, const Vector *two, vector_item_comparitor comparitor);
bool    vector_iterate(const Vector *vector, vector_iterator iterator, void *context);

Vector *vector_map(const Vector *vector, vector_map_function function, void *context);
Vector *vector_map_into(const Vector *vector, vector_map_function function, void *context, Vector *target);

