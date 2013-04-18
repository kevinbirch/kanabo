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
#include "conditions.h"

static bool slice_predicate_has(const predicate * restrict value, enum slice_specifiers specifier);

enum path_kind path_get_kind(const jsonpath * restrict path)
{
    if(NULL == path)
    {
        errno = EINVAL;
        return (enum path_kind)-1;
    }
    return path->kind;
}

size_t path_get_length(const jsonpath * restrict path)
{
    PRECOND_NONNULL_ELSE_ZERO(path);
    return path->length;
}

step *path_get_step(const jsonpath * restrict path, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(path, path->steps);
    PRECOND_ELSE_NULL(0 != path->length, index < path->length);
    return path->steps[index];
}

enum step_kind step_get_kind(const step * restrict value)
{
    if(NULL == value)
    {
        errno = EINVAL;
        return (enum step_kind)-1;
    }
    return value->kind;
}

enum test_kind step_get_test_kind(const step * restrict value)
{
    if(NULL == value)
    {
        errno = EINVAL;
        return (enum test_kind)-1;
    }
    return value->test.kind;
}

enum type_test_kind type_test_step_get_type(const step * restrict value)
{
    if(NULL == value || NAME_TEST == value->test.kind)
    {
        errno = EINVAL;
        return (enum type_test_kind)-1;
    }
    return value->test.type;
}

uint8_t *name_test_step_get_name(const step * restrict value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_ELSE_NULL(NAME_TEST == value->test.kind);
    return value->test.name.value;
}

size_t name_test_step_get_length(const step * restrict value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_ELSE_NULL(NAME_TEST == value->test.kind);
    return value->test.name.length;
}

bool step_has_predicate(const step * restrict value)
{
    PRECOND_NONNULL_ELSE_FALSE(value);
    return NULL != value->predicate;
}

predicate *step_get_predicate(const step * restrict value)
{
    PRECOND_NONNULL_ELSE_NULL(value, value->predicate);
    return value->predicate;
}

enum predicate_kind predicate_get_kind(const predicate * restrict value)
{
    if(NULL == value)
    {
        errno = EINVAL;
        return (enum predicate_kind)-1;
    }
    return value->kind;
}

size_t subscript_predicate_get_index(const predicate * restrict value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SUBSCRIPT == value->kind);
    return value->subscript.index;
}

int_fast32_t slice_predicate_get_to(const predicate * restrict value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.to;
}

int_fast32_t slice_predicate_get_from(const predicate * restrict value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.from;
}

int_fast32_t slice_predicate_get_step(const predicate * restrict value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.step;
}
bool slice_predicate_has_to(const predicate * restrict value)
{
    return slice_predicate_has(value, SLICE_TO);
}

bool slice_predicate_has_from(const predicate * restrict value)
{
    return slice_predicate_has(value, SLICE_FROM);
}

bool slice_predicate_has_step(const predicate * restrict value)
{
    return slice_predicate_has(value, SLICE_STEP);
}

static bool slice_predicate_has(const predicate * restrict value, enum slice_specifiers specifier)
{
    PRECOND_NONNULL_ELSE_FALSE(value);
    PRECOND_ELSE_FALSE(SLICE == value->kind);
    return value->slice.specified & specifier;
}

jsonpath *join_predicate_get_left(const predicate * restrict value)
{
    if(NULL == value || JOIN != value->kind)
    {
        errno = EINVAL;
        return NULL;
    }
    return value->join.left;
}

jsonpath *join_predicate_get_right(const predicate * restrict value)
{
    if(NULL == value || JOIN != value->kind)
    {
        errno = EINVAL;
        return NULL;
    }
    return value->join.right;
}
