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
#include <errno.h>

#include "model.h"
#include "conditions.h"

static inline node *make_node(enum node_kind kind);

static bool scalar_comparitor(const void *one, const void *two);
static hashcode scalar_hash(const void *key);


document_model *make_model(void)
{
    document_model *result = (document_model *)calloc(1, sizeof(document_model));
    if(NULL != result)
    {
        result->documents = make_vector();
        if(NULL == result->documents)
        {
            free(result);
            result = NULL;
            return NULL;
        }
    }

    return result;
}

node *make_document_node(void)
{
    node *result = make_node(DOCUMENT);
    if(NULL != result)
    {
        result->content.size = 0;
    }

    return result;
}

node *make_mapping_node(void)
{
    node *result = make_node(MAPPING);
    if(NULL != result)
    {
        result->content.mapping = make_hashtable_with_function(scalar_comparitor, scalar_hash);
        if(NULL == result->content.mapping)
        {
            free(result);
            result = NULL;
            return NULL;
        }        
        result->content.size = 0;
    }

    return result;
}    

node *make_scalar_node(const uint8_t *value, size_t length, enum scalar_kind kind)
{
    if(NULL == value && 0 != length)
    {
        errno = EINVAL;
        return NULL;
    }

    node *result = make_node(SCALAR);
    if(NULL != result)
    {
        result->content.size = length;
        result->content.scalar.kind = kind;
        if(NULL != value)
        {
            result->content.scalar.value = (uint8_t *)calloc(1, length);
            if(NULL == result->content.scalar.value)
            {
                free(result);
                result = NULL;
                return NULL;
            }
            memcpy(result->content.scalar.value, value, length);
        }
    }

    return result;
}

node *make_alias_node(node *target)
{
    node *result = make_node(ALIAS);
    if(NULL != result)
    {
        result->content.size = 0;
        result->content.target = target;
    }

    return result;
}

static inline node *make_node(enum node_kind kind)
{
    node *result = (node *)calloc(1, sizeof(struct node));
    if(NULL != result)
    {
        result->tag.kind = kind;
        result->tag.name = NULL;
        result->anchor = NULL;
    }

    return result;
}

static hashcode scalar_hash(const void *key)
{
    node *scalar = (node *)key;
    return shift_add_xor_string_buffer_hash(scalar_value(scalar), node_size(scalar));
}

static bool scalar_comparitor(const void *one, const void *two)
{
    return node_equals((node *)one, (node *)two);
}
