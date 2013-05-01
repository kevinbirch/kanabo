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

static bool model_init(document_model * restrict model, size_t capacity);

static inline node *make_node(enum node_kind kind);

static inline void sequence_free(node *sequence);
static inline void mapping_free(node *mapping);

node *make_document_node(node *root)
{
    if(NULL == root)
    {
        errno = EINVAL;
        return NULL;
    }

    node *result = make_node(DOCUMENT);
    if(NULL != result)
    {
        result->content.size = 1;
        result->content.document.root = root;
    }

    return result;
}

node *make_sequence_node(size_t capacity)
{
    if(0 == capacity)
    {
        errno = EINVAL;
        return NULL;
    }

    node *result = make_node(SEQUENCE);

    if(NULL != result)
    {
        result->content.size = 0;
        result->content.sequence.capacity = capacity;
        result->content.sequence.value = (node **)calloc(1, sizeof(node *) * capacity);
        if(NULL == result->content.sequence.value)
        {
            free(result);
            result = NULL;
            return NULL;
        }
    }

    return result;
}    

node *make_mapping_node(size_t capacity)
{
    if(0 == capacity)
    {
        errno = EINVAL;
        return NULL;
    }

    node *result = make_node(MAPPING);
    if(NULL != result)
    {
        result->content.size = 0;
        result->content.mapping.capacity = capacity;
        result->content.mapping.value = (key_value_pair **)calloc(1, sizeof(key_value_pair *) * capacity);
        if(NULL == result->content.mapping.value)
        {
            free(result);
            result = NULL;
            return NULL;
        }        
    }

    return result;
}    

node *make_scalar_node(const uint8_t *value, size_t length, enum scalar_kind kind)
{
    if(NULL == value || 0 == length)
    {
        errno = EINVAL;
        return NULL;
    }

    node *result = make_node(SCALAR);
    if(NULL != result)
    {
        result->content.size = length;
        result->content.scalar.kind = kind;
        result->content.scalar.value = (uint8_t *)calloc(1, length);
        if(NULL == result->content.scalar.value)
        {
            free(result);
            result = NULL;
            return NULL;
        }
        memcpy(result->content.scalar.value, value, length);
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
        result->tag.name_length = 0;
    }

    return result;
}

void model_free(document_model *model)
{
    if(NULL == model)
    {
        return;
    }
    for(size_t i = 0; i < model->size; i++)
    {
        node_free(model->documents[i]);
        model->documents[i] = NULL;
    }
    model->size = 0;
    model->documents = NULL;
}

void node_free(node *value)
{
    if(NULL == value)
    {
        return;
    }
    switch(node_get_kind(value))
    {
        case DOCUMENT:
            node_free(value->content.document.root);
            value->content.document.root = NULL;
            break;
        case SCALAR:
            free(value->content.scalar.value);
            value->content.scalar.value = NULL;
            break;
        case SEQUENCE:
            sequence_free(value);
            break;
        case MAPPING:
            mapping_free(value);
            break;
    }
    free(value);
}

static inline void sequence_free(node *sequence)
{
    if(NULL == sequence_get_all(sequence))
    {
        return;
    }
    for(size_t i = 0; i < node_get_size(sequence); i++)
    {
        node_free(sequence->content.sequence.value[i]);
        sequence->content.sequence.value[i] = NULL;
    }
    free(sequence->content.sequence.value);
    sequence->content.sequence.value = NULL;
}

static inline void mapping_free(node *mapping)
{
    if(NULL == mapping->content.mapping.value)
    {
        return;
    }
    for(size_t i = 0; i < node_get_size(mapping); i++)
    {
        node_free(mapping->content.mapping.value[i]->key);
        mapping->content.mapping.value[i]->key = NULL;
        node_free(mapping->content.mapping.value[i]->value);
        mapping->content.mapping.value[i]->value = NULL;
        free(mapping->content.mapping.value[i]);
        mapping->content.mapping.value[i] = NULL;
    }
    free(mapping->content.mapping.value);
    mapping->content.mapping.value = NULL;
}

document_model *make_model(size_t capacity)
{
    document_model *result = (document_model *)calloc(1, sizeof(document_model));
    bool initialized = false;
    if(NULL != result)
    {
        initialized = model_init(result, capacity);
    }

    return NULL != result && initialized ? result : NULL;
}

static bool model_init(document_model * restrict model, size_t capacity)
{
    if(NULL == model || 0 == capacity)
    {
        errno = EINVAL;
        return false;
    }

    bool result = true;
    model->size = 0;
    model->documents = (node **)calloc(1, sizeof(node *) * capacity);
    if(NULL == model->documents)
    {
        model->capacity = 0;
        result = false;
    }
    else
    {
        model->capacity = capacity;
    }

    return result;
}

