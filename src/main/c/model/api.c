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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "model.h"
#include "model/private.h"
#include "conditions.h"

struct context_adapter_s
{
    mapping_iterator iterator;
    void *context;
};

typedef struct context_adapter_s context_adapter;

static bool tag_equals(const uint8_t * restrict one, const uint8_t * restrict two);
static bool sequence_equals(const node * restrict one, const node * restrict two);
static bool mapping_equals(const node * restrict one, const node * restrict two);

static bool iterator_adpater(void *key, void *value, void *context);


node *model_document(const document_model * restrict model, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(model);
    PRECOND_ELSE_NULL(index < model_document_count(model));

    return model->documents[index];
}

node *model_document_root(const document_model * restrict model, size_t index)
{
    node *document = model_document(model, index);
    node *result = NULL;
    
    if(NULL != document)
    {
        result = document_root(document);
    }

    return result;
}

size_t model_document_count(const document_model * restrict model)
{
    PRECOND_NONNULL_ELSE_ZERO(model);

    return model->size;
}

enum node_kind node_kind(const node * restrict value)
{
    return value->tag.kind;
}

uint8_t *node_name(const node * restrict value)
{
    PRECOND_NONNULL_ELSE_NULL(value);

    return value->tag.name;
}

size_t node_size(const node * restrict value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);

    return value->content.size;
}

node *document_root(const node * restrict document)
{
    PRECOND_NONNULL_ELSE_NULL(document);
    PRECOND_ELSE_NULL(DOCUMENT == node_kind(document));

    return document->content.document.root;
}

uint8_t *scalar_value(const node * restrict scalar)
{
    PRECOND_NONNULL_ELSE_NULL(scalar);
    PRECOND_ELSE_NULL(SCALAR == node_kind(scalar));

    return scalar->content.scalar.value;
}

enum scalar_kind scalar_kind(const node * restrict scalar)
{
    return scalar->content.scalar.kind;
}

bool scalar_boolean_is_true(const node * restrict scalar)
{
    return 0 == memcmp("true", scalar_value(scalar), 4);
}

bool scalar_boolean_is_false(const node * restrict scalar)
{
    return 0 == memcmp("false", scalar_value(scalar), 5);
}

node *sequence_get(const node * restrict sequence, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(sequence);
    PRECOND_ELSE_NULL(SEQUENCE == node_kind(sequence));
    PRECOND_ELSE_NULL(index < node_size(sequence));

    return sequence->content.sequence.value[index];
}

node **sequence_get_all(const node * restrict sequence)
{
    PRECOND_NONNULL_ELSE_NULL(sequence);
    PRECOND_ELSE_NULL(SEQUENCE == node_kind(sequence));

    return sequence->content.sequence.value;
}

bool sequence_iterate(const node * restrict sequence, sequence_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(sequence, iterator);
    PRECOND_ELSE_FALSE(SEQUENCE == node_kind(sequence));

    for(size_t i = 0; i < node_size(sequence); i++)
    {
        if(!iterator(sequence->content.sequence.value[i], context))
        {
            return false;
        }
    }
    return true;
}

node *mapping_get(const node * restrict mapping, uint8_t *key, size_t length)
{
    PRECOND_NONNULL_ELSE_NULL(mapping, key);
    PRECOND_ELSE_NULL(MAPPING == node_kind(mapping));
    PRECOND_ELSE_NULL(0 < length);

    node *scalar = make_scalar_node(key, length, SCALAR_STRING);
    node *result = hashtable_get(mapping->content.mapping, (void *)scalar);
    node_free(scalar);
    
    return result;
}

bool mapping_contains(const node * restrict mapping, uint8_t *key, size_t length)
{
    PRECOND_NONNULL_ELSE_NULL(mapping, key);
    PRECOND_ELSE_NULL(MAPPING == node_kind(mapping));

    node *scalar = make_scalar_node(key, length, SCALAR_STRING);
    bool result = hashtable_contains(mapping->content.mapping, scalar);
    node_free(scalar);

    return result;
}

static bool iterator_adpater(void *key, void *value, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator((node *)key, (node *)value, adapter->context);
}

bool mapping_iterate(const node * restrict mapping, mapping_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, iterator);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping));

    return hashtable_iterate(mapping->content.mapping, iterator_adpater,  &(context_adapter){iterator, context});
}

bool node_equals(const node * restrict one, const node * restrict two)
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
    }
    return result;
}

bool scalar_equals(const node * restrict one, const node * restrict two)
{
    size_t n1 = node_size(one);
    size_t n2 = node_size(two);

    if(n1 != n2)
    {
        return false;
    }
    return memcmp(scalar_value(one), scalar_value(two), n1) == 0;
}

static bool tag_equals(const uint8_t * restrict one, const uint8_t * restrict two)
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

static bool sequence_equals(const node * restrict one, const node * restrict two)
{
    for(size_t i = node_size(one); i < node_size(one); i++)
    {
        if(!node_equals(one->content.sequence.value[i], two->content.sequence.value[i]))
        {
            return false;
        }
    }
    return true;
}

bool node_comparitor(void *one, void *two)
{
    return node_equals((node *)one, (node *)two);
}

static bool mapping_equals(const node * restrict one, const node * restrict two)
{
    return hashtable_equals(one->content.mapping, two->content.mapping, node_comparitor);
}


