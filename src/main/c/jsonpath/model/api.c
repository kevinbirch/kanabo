/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 *
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README file in the project root.
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

#include <errno.h>

#include "jsonpath.h"
#include "jsonpath/private.h"
#include "conditions.h"

static bool slice_predicate_has(const predicate *value, enum slice_specifiers specifier);

static void step_free(step *step);
static void predicate_free(predicate *predicate);

void path_free(jsonpath *path)
{
    if(NULL == path || NULL == path->steps || 0 == path->length)
    {
        return;
    }
    for(size_t i = 0; i < path->length; i++)
    {
        step_free(path->steps[i]);
    }
    free(path->steps);
    free(path->expression);
    free(path);
}

void step_free(step *value)
{
    if(NULL == value)
    {
        return;
    }
    if(NAME_TEST == value->test.kind)
    {
        if(NULL != value->test.name.value)
        {
            free(value->test.name.value);
            value->test.name.value = NULL;
            value->test.name.length = 0;
        }
    }
    if(NULL != value->predicate)
    {
        predicate_free(value->predicate);
    }
    free(value);
}

static void predicate_free(predicate *value)
{
    if(NULL == value)
    {
        return;
    }
    if(JOIN == predicate_kind(value))
    {
        path_free(value->join.left);
        path_free(value->join.right);
    }

    free(value);
}

bool path_iterate(const jsonpath *path, path_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(path, iterator);

    for(size_t i = 0; i < path_length(path); i++)
    {
        if(!iterator(path->steps[i], context))
        {
            return false;
        }
    }
    return true;
}

uint8_t *path_expression(const jsonpath *path)
{
    PRECOND_NONNULL_ELSE_NULL(path);
    return path->expression;
}

size_t path_expression_length(const jsonpath *path)
{
    PRECOND_NONNULL_ELSE_ZERO(path);
    return path->expr_length;
}

enum path_kind path_kind(const jsonpath *path)
{
    return path->kind;
}

size_t path_length(const jsonpath *path)
{
    PRECOND_NONNULL_ELSE_ZERO(path);
    return path->length;
}

step *path_get(const jsonpath *path, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(path);
    PRECOND_NONNULL_ELSE_NULL(path->steps);
    PRECOND_ELSE_NULL(0 != path->length, index < path->length);
    return path->steps[index];
}

enum step_kind step_kind(const step *value)
{
    return value->kind;
}

enum test_kind step_test_kind(const step *value)
{
    return value->test.kind;
}

enum type_test_kind type_test_step_kind(const step *value)
{
    return value->test.type;
}

uint8_t *name_test_step_name(const step *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_ELSE_NULL(NAME_TEST == value->test.kind);
    return value->test.name.value;
}

size_t name_test_step_length(const step *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(NAME_TEST == value->test.kind);
    return value->test.name.length;
}

bool step_has_predicate(const step *value)
{
    PRECOND_NONNULL_ELSE_FALSE(value);
    return NULL != value->predicate;
}

predicate *step_predicate(const step *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_NONNULL_ELSE_NULL(value->predicate);
    return value->predicate;
}

enum predicate_kind predicate_kind(const predicate *value)
{
    return value->kind;
}

size_t subscript_predicate_index(const predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SUBSCRIPT == value->kind);
    return value->subscript.index;
}

int_fast32_t slice_predicate_to(const predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.to;
}

int_fast32_t slice_predicate_from(const predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.from;
}

int_fast32_t slice_predicate_step(const predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.step;
}

bool slice_predicate_has_to(const predicate *value)
{
    return slice_predicate_has(value, SLICE_TO);
}

bool slice_predicate_has_from(const predicate *value)
{
    return slice_predicate_has(value, SLICE_FROM);
}

bool slice_predicate_has_step(const predicate *value)
{
    return slice_predicate_has(value, SLICE_STEP);
}

static bool slice_predicate_has(const predicate *value, enum slice_specifiers specifier)
{
    PRECOND_NONNULL_ELSE_FALSE(value);
    PRECOND_ELSE_FALSE(SLICE == value->kind);
    return value->slice.specified & specifier;
}

jsonpath *join_predicate_left(const predicate *value)
{
    if(NULL == value || JOIN != value->kind)
    {
        errno = EINVAL;
        return NULL;
    }
    return value->join.left;
}

jsonpath *join_predicate_right(const predicate *value)
{
    if(NULL == value || JOIN != value->kind)
    {
        errno = EINVAL;
        return NULL;
    }
    return value->join.right;
}
