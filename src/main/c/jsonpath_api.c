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

enum path_kind path_get_kind(const jsonpath * restrict path)
{
    if(NULL == path)
    {
        errno = EINVAL;
        return (enum path_kind)-1;
    }
    errno = 0;
    return path->kind;
}

size_t path_get_length(const jsonpath * restrict path)
{
    if(NULL == path)
    {
        errno = EINVAL;
        return 0;
    }
    errno = 0;
    return path->length;
}

step *path_get_step(const jsonpath * restrict path, size_t index)
{
    if(NULL == path || 0 == path->length || NULL == path->steps || index >= path->length)
    {
        errno = EINVAL;
        return NULL;
    }
    errno = 0;
    return path->steps[index];
}

enum step_kind step_get_kind(const step * restrict step)
{
    if(NULL == step)
    {
        errno = EINVAL;
        return (enum step_kind)-1;
    }
    errno = 0;
    return step->kind;
}

enum test_kind step_get_test_kind(const step * restrict step)
{
    if(NULL == step)
    {
        errno = EINVAL;
        return (enum test_kind)-1;
    }
    errno = 0;
    return step->test.kind;
}

enum type_test_kind type_test_step_get_type(const step * restrict step)
{
    if(NULL == step || NAME_TEST == step->test.kind)
    {
        errno = EINVAL;
        return (enum type_test_kind)-1;
    }
    errno = 0;
    return step->test.type;
}

uint8_t *name_test_step_get_name(const step * restrict step)
{
    if(NULL == step || TYPE_TEST == step->test.kind)
    {
        errno = EINVAL;
        return NULL;
    }
    errno = 0;
    return step->test.name.value;
}

size_t name_test_step_get_length(const step * restrict step)
{
    if(NULL == step || TYPE_TEST == step->test.kind)
    {
        errno = EINVAL;
        return 0;
    }
    errno = 0;
    return step->test.name.length;
}

bool step_has_predicate(const step * restrict step)
{
    if(NULL == step)
    {
        errno = EINVAL;
        return false;
    }
    errno = 0;
    return NULL != step->predicate;
}

predicate *step_get_predicate(const step * restrict step)
{
    if(NULL == step || NULL == step->predicate)
    {
        errno = EINVAL;
        return NULL;
    }
    errno = 0;
    return step->predicate;
}

enum predicate_kind predicate_get_kind(const predicate * restrict predicate)
{
    if(NULL == predicate)
    {
        errno = EINVAL;
        return (enum predicate_kind)-1;
    }
    errno = 0;
    return predicate->kind;
}

uint_fast32_t subscript_predicate_get_index(const predicate * restrict predicate)
{
    if(NULL == predicate || SUBSCRIPT != predicate->kind)
    {
        errno = EINVAL;
        return 0;
    }
    errno = 0;
    return predicate->subscript.index;
}

uint_fast32_t slice_predicate_get_to(const predicate * restrict predicate)
{
    if(NULL == predicate || SLICE != predicate->kind)
    {
        errno = EINVAL;
        return 0;
    }
    errno = 0;
    return predicate->slice.to;
}

uint_fast32_t slice_predicate_get_from(const predicate * restrict predicate)
{
    if(NULL == predicate || SLICE != predicate->kind)
    {
        errno = EINVAL;
        return 0;
    }
    errno = 0;
    return predicate->slice.from;
}

uint_fast32_t slice_predicate_get_step(const predicate * restrict predicate)
{
    if(NULL == predicate || SLICE != predicate->kind)
    {
        errno = EINVAL;
        return 0;
    }
    errno = 0;
    return predicate->slice.step;
}

jsonpath *join_predicate_get_left(const predicate * restrict predicate)
{
    if(NULL == predicate || JOIN != predicate->kind)
    {
        errno = EINVAL;
        return NULL;
    }
    errno = 0;
    return predicate->join.left;
}

jsonpath *join_predicate_get_right(const predicate * restrict predicate)
{
    if(NULL == predicate || JOIN != predicate->kind)
    {
        errno = EINVAL;
        return NULL;
    }
    errno = 0;
    return predicate->join.right;
}
