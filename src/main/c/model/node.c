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

#include <string.h>

#include "model.h"
#include "conditions.h"

static bool node_comparitor(const void *one, const void *two);
static bool tag_equals(const uint8_t *one, const uint8_t *two);

static bool scalar_equals(const node *one, const node *two);
static bool sequence_equals(const node *one, const node *two);
static bool mapping_equals(const node *one, const node *two);


enum node_kind node_kind(const node *value)
{
    return value->tag.kind;
}

uint8_t *node_name(const node *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);

    return value->tag.name;
}

size_t node_size(const node *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);

    return value->content.size;
}

node *node_parent(const node *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    return value->parent;
}

void node_set_tag(node *target, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->tag.name = (uint8_t *)calloc(1, length + 1);
    if(NULL != target->tag.name)
    {
        memcpy(target->tag.name, value, length);
        target->tag.name[length] = '\0';
    }
}

void node_set_anchor(node *target, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->anchor = (uint8_t *)calloc(1, length + 1);
    if(NULL != target->anchor)
    {
        memcpy(target->anchor, value, length);
        target->anchor[length] = '\0';
    }
}

bool node_equals(const node *one, const node *two)
{
    if(one == two)
    {
        return true;
    }
    
    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }

    bool result = node_kind(one) == node_kind(two) &&
        tag_equals(node_name(one), node_name(two)) &&
        node_size(one) == node_size(two);

    if(!result)
    {
        return result;
    }
    switch(node_kind(one))
    {
        case DOCUMENT:
            result &= node_equals(document_root(one), document_root(two));
            break;
        case SCALAR:
            result &= scalar_equals(one, two);
            break;
        case SEQUENCE:
            result &= sequence_equals(one, two);
            break;
        case MAPPING:
            result &= mapping_equals(one, two);
            break;
        case ALIAS:
            result &= node_equals(alias_target(one), alias_target(two));
            break;
    }
    return result;
}

static bool scalar_equals(const node *one, const node *two)
{
    size_t n1 = node_size(one);
    size_t n2 = node_size(two);

    if(n1 != n2)
    {
        return false;
    }
    return memcmp(scalar_value(one), scalar_value(two), n1) == 0;
}

static bool tag_equals(const uint8_t *one, const uint8_t *two)
{
    if(NULL == one && NULL == two)
    {
        return true;
    }
    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }
    size_t n1 = strlen((char *)one);
    size_t n2 = strlen((char *)two);
    return memcmp(one, two, n1 > n2 ? n2 : n1) == 0;
}

static bool node_comparitor(const void *one, const void *two)
{
    return node_equals((node *)one, (node *)two);
}

static bool sequence_equals(const node *one, const node *two)
{
    return vector_equals(one->content.sequence, two->content.sequence, node_comparitor);
}

static bool mapping_equals(const node *one, const node *two)
{
    return hashtable_equals(one->content.mapping, two->content.mapping, node_comparitor);
}
