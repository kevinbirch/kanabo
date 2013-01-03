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

static bool node_equals(const node *one, const node *two);
static bool string_equals(const uint8_t * restrict one, size_t n1, const uint8_t * restrict two, size_t n2);
static bool sequence_equals(const node * restrict one, const node * restrict two);
static bool mapping_equals(const node * restrict one, const node * restrict two);

static inline node *make_node(enum node_kind kind);

static inline void free_sequence(node *sequence);
static inline void free_mapping(node *mapping);

node *model_get_document(const document_model * restrict model, size_t index)
{
    if(NULL == model || index > (model_get_document_count(model) - 1))
    {
        errno = EINVAL;
        return NULL;
    }

    errno = 0;
    return model->documents[index];
}

node *model_get_document_root(const document_model * restrict model, size_t index)
{
    errno = 0;
    node *document = model_get_document(model, index);
    node *result = NULL;
    
    if(NULL != document)
    {
        result = document_get_root(document);
    }

    return result;
}

size_t model_get_document_count(const document_model * restrict model)
{
    if(NULL == model)
    {
        errno = EINVAL;
        return 0;
    }

    errno = 0;
    return model->size;
}

enum node_kind node_get_kind(const node * restrict node)
{
    if(NULL == node)
    {
        errno = EINVAL;
        return (enum node_kind)-1;
    }

    errno = 0;
    return node->tag.kind;
}

uint8_t *node_get_name(const node * restrict node)
{
    if(NULL == node)
    {
        errno = EINVAL;
        return NULL;
    }

    errno = 0;
    return node->tag.name;
}

size_t node_get_name_length(const node * restrict node)
{
    if(NULL == node)
    {
        errno = EINVAL;
        return 0;
    }

    errno = 0;
    return node->tag.name_length;
}

size_t node_get_size(const node * restrict node)
{
    if(NULL == node)
    {
        errno = EINVAL;
        return 0;
    }

    errno = 0;
    return node->content.size;
}

node *document_get_root(const node * restrict document)
{
    if(NULL == document || DOCUMENT != node_get_kind(document))
    {
        errno = EINVAL;
        return NULL;
    }    

    errno = 0;
    return document->content.document.root;
}

uint8_t *scalar_get_value(const node * restrict scalar)
{
    if(NULL == scalar || SCALAR != node_get_kind(scalar))
    {
        errno = EINVAL;
        return NULL;
    }    

    errno = 0;
    return scalar->content.scalar.value;
}

node *sequence_get_item(const node * restrict sequence, size_t index)
{
    if(NULL == sequence || SEQUENCE != node_get_kind(sequence) || index > (node_get_size(sequence) - 1))
    {
        errno = EINVAL;
        return NULL;
    }

    errno = 0;
    return sequence->content.sequence.value[index];
}

node **sequence_get_all(const node * restrict sequence)
{
    if(NULL == sequence || SEQUENCE != node_get_kind(sequence))
    {
        errno = EINVAL;
        return NULL;
    }

    errno = 0;
    return sequence->content.sequence.value;
}

void iterate_sequence(const node * restrict sequence, sequence_iterator iterator, void *context)
{
    if(NULL == sequence || SEQUENCE != node_get_kind(sequence) || NULL == iterator)
    {
        errno = EINVAL;
        return;
    }
    for(size_t i = 0; i < node_get_size(sequence); i++)
    {
        iterator(sequence->content.sequence.value[i], context);
    }
}

node *mapping_get_value(const node * restrict mapping, const char *key)
{
    if(NULL == mapping || MAPPING != node_get_kind(mapping) || NULL == key)
    {
        errno = EINVAL;
        return NULL;
    }

    return mapping_get_value_scalar_key(mapping, (uint8_t *)key, strlen(key));
}

node *mapping_get_value_scalar_key(const node * restrict mapping, uint8_t *key, size_t key_length)
{
    if(NULL == mapping || MAPPING != node_get_kind(mapping) || NULL == key || 0 == key_length)
    {
        errno = EINVAL;
        return NULL;
    }

    node *scalar = make_scalar_node(key, key_length);
    return mapping_get_value_node_key(mapping, scalar);
}

node *mapping_get_value_node_key(const node * restrict mapping, const node *key)
{
    if(NULL == mapping || MAPPING != node_get_kind(mapping) || NULL == key)
    {
        errno = EINVAL;
        return NULL;
    }
    node *result = NULL;
    for(size_t i = 0; i < mapping->content.size; i++)
    {
        if(node_equals(key, mapping->content.mapping.value[i]->key))
        {
            result = mapping->content.mapping.value[i]->value;
            break;
        }
    }

    errno = 0;
    return result;
}

bool mapping_contains_key(const node * restrict mapping, const char *key)
{
    return NULL != mapping_get_value(mapping, key);
}

bool mapping_contains_node_key(const node * restrict mapping, const node *key)
{
    return NULL != mapping_get_value_node_key(mapping, key);
}

key_value_pair **mapping_get_all(const node * restrict mapping)
{
    if(NULL == mapping || MAPPING != node_get_kind(mapping))
    {
        errno = EINVAL;
        return NULL;
    }

    errno = 0;
    return mapping->content.mapping.value;
}

void iterate_mapping(const node * restrict mapping, mapping_iterator iterator, void *context)
{
    if(NULL == mapping || MAPPING != node_get_kind(mapping) || NULL == iterator)
    {
        errno = EINVAL;
        return;
    }
    for(size_t i = 0; i < node_get_size(mapping); i++)
    {
        iterator(mapping->content.mapping.value[i]->key, mapping->content.mapping.value[i]->value, context);
    }
}

static bool node_equals(const node * restrict one, const node * restrict two)
{
    if(one == two)
    {
        return true;
    }
    
    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }

    bool result = 
        node_get_kind(one) == node_get_kind(two) &&
        node_get_name_length(one) == node_get_name_length(two) &&
        string_equals(node_get_name(one), node_get_name_length(one), node_get_name(two), node_get_name_length(two)) &&
        node_get_size(one) == node_get_size(two);
    
    switch(node_get_kind(one))
    {
        case DOCUMENT:
            result &= node_equals(document_get_root(one), document_get_root(two));
            break;
        case SCALAR:
            result &= string_equals(scalar_get_value(one), node_get_size(one), scalar_get_value(two), node_get_size(two));
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

static bool string_equals(const uint8_t * restrict one, size_t n1, const uint8_t * restrict two, size_t n2)
{
    return memcmp(one, two, n1 > n2 ? n2 : n1) == 0;
}

static bool sequence_equals(const node * restrict one, const node * restrict two)
{
    for(size_t i = node_get_size(one); i < node_get_size(one); i++)
    {
        if(!node_equals(one->content.sequence.value[i], two->content.sequence.value[i]))
        {
            return false;
        }
    }
    return true;
}

static bool mapping_equals(const node * restrict one, const node * restrict two)
{
    for(size_t i = node_get_size(one); i < node_get_size(one); i++)
    {
        if(!node_equals(one->content.mapping.value[i]->key, two->content.mapping.value[i]->key) ||
           !node_equals(one->content.mapping.value[i]->value, two->content.mapping.value[i]->value))
        {
            return false;
        }
    }
    return true;
}

node *make_document_node(node *root)
{
    if(NULL == root)
    {
        errno = EINVAL;
        return NULL;
    }

    errno = 0;
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

    errno = 0;
    node *result = make_node(SEQUENCE);

    if(NULL != result)
    {
        result->content.size = 0;
        result->content.sequence.capacity = capacity;
        result->content.sequence.value = (node **)malloc(sizeof(node *) * capacity);
        if(NULL == result->content.sequence.value)
        {
            free(result);
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

    errno = 0;
    node *result = make_node(MAPPING);
    if(NULL != result)
    {
        result->content.size = 0;
        result->content.mapping.capacity = capacity;
        result->content.mapping.value = (key_value_pair **)malloc(sizeof(key_value_pair *) * capacity);
        if(NULL == result->content.mapping.value)
        {
            free(result);
            return NULL;
        }        
    }

    return result;
}    

node *make_scalar_node(const uint8_t *value, size_t length)
{
    if(NULL == value || 0 == length)
    {
        errno = EINVAL;
        return NULL;
    }

    errno = 0;
    node *result = make_node(SCALAR);
    if(NULL != result)
    {
        result->content.size = length;
        result->content.scalar.value = (uint8_t *)malloc(length);
        if(NULL == result->content.scalar.value)
        {
            free(result);
            return NULL;
        }
        memcpy(result->content.scalar.value, value, length);
    }

    return result;
}

static inline node *make_node(enum node_kind kind)
{
    errno = 0;
    node *result = (node *)malloc(sizeof(struct node));

    if(NULL != result)
    {
        result->tag.kind = kind;
        result->tag.name = NULL;
        result->tag.name_length = 0;
    }

    return result;
}

void free_model(document_model *model)
{
    if(NULL == model)
    {
        return;
    }
    for(size_t i = 0; i < model->size; i++)
    {
        free_node(model_get_document(model, i));
    }
    model->size = 0;
    model->documents = NULL;
}

void free_node(node *value)
{
    if(NULL == value)
    {
        return;
    }
    switch(node_get_kind(value))
    {
        case DOCUMENT:
            free_node(document_get_root(value));
            break;
        case SCALAR:
            free(scalar_get_value(value));
            break;
        case SEQUENCE:
            free_sequence(value);
            break;
        case MAPPING:
            free_mapping(value);
            break;
    }
    free(value);
}

static inline void free_sequence(node *sequence)
{
    if(NULL == sequence_get_all(sequence))
    {
        return;
    }
    for(size_t i = 0; i < node_get_size(sequence); i++)
    {
        free_node(sequence_get_item(sequence, i));
    }
    free(sequence_get_all(sequence));
}

static inline void free_mapping(node *mapping)
{
    key_value_pair **pairs = mapping_get_all(mapping);
    if(NULL == pairs)
    {
        return;
    }
    for(size_t i = 0; i < node_get_size(mapping); i++)
    {
        free_node(pairs[i]->key);
        free_node(pairs[i]->value);
        free(pairs[i]);
    }
    free(pairs);
}

document_model *make_model(size_t capacity)
{
    document_model *result = (document_model *)malloc(sizeof(document_model));
    bool initialized = false;
    if(NULL != result)
    {
        initialized = init_model(result, capacity);
    }

    return NULL != result && initialized ? result : NULL;
}

bool init_model(document_model * restrict model, size_t capacity)
{
    if(NULL == model || 0 == capacity)
    {
        errno = EINVAL;
        return false;
    }

    errno = 0;
    bool result = true;
    model->size = 0;
    model->documents = (node **)malloc(sizeof(node *) * capacity);
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

#define ensure_capacity(T, A, S, C)                                     \
    if((S) > (C))                                                       \
    {                                                                   \
        size_t size_plus_50_percent = ((S) * 3) / 2 + 1;                \
        T **old = (A);                                                  \
        (A) = (T **)malloc(sizeof(T *) * size_plus_50_percent);         \
        if(NULL == (A))                                                 \
        {                                                               \
            return false;                                               \
        }                                                               \
        memcpy((A), old, sizeof(T *) * (S));                            \
        free(old);                                                      \
        (C) = size_plus_50_percent;                                     \
    }

bool model_add(document_model * restrict model, node *document)
{
    if(NULL == model || NULL == document || DOCUMENT != node_get_kind(document))
    {
        errno = EINVAL;
        return false;
    }

    errno = 0;
    bool result = true;
    ensure_capacity(node, model->documents, model->size + 1, model->capacity);
    model->documents[model->size++] = document;
    return result;
}

bool document_set_root(node * restrict document, node *root)
{
    if(NULL == document || DOCUMENT != node_get_kind(document) || NULL == root)
    {
        errno = EINVAL;
        return false;
    }

    errno = 0;
    document->content.document.root = root;
    return true;
}

bool sequence_add(node * restrict sequence, node *item)
{
    if(NULL == sequence || SEQUENCE != node_get_kind(sequence) || NULL == item)
    {
        errno = EINVAL;
        return false;
    }

    errno = 0;
    bool result = true;
    ensure_capacity(node, sequence->content.sequence.value, sequence->content.size + 1, sequence->content.sequence.capacity);
    sequence->content.sequence.value[sequence->content.size++] = item;

    return result;
}

bool sequence_add_all(node * restrict sequence, node **items, size_t count)
{
    if(NULL == sequence || SEQUENCE != node_get_kind(sequence) || NULL == items || 0 == count)
    {
        errno = EINVAL;
        return false;
    }

    errno = 0;
    bool result = true;
    ensure_capacity(node, sequence->content.sequence.value, count, sequence->content.sequence.capacity);
    for(size_t i = 0; i < count; i++)
    {
        sequence->content.sequence.value[sequence->content.size++] = items[i];
    }
    return result;
}

bool mapping_put(node * restrict mapping, node *key, node *value)
{
    if(NULL == mapping || MAPPING != node_get_kind(mapping) || NULL == key || NULL == value)
    {
        errno = EINVAL;
        return false;
    }

    errno = 0;
    bool result = true;
    key_value_pair *pair = (key_value_pair *)malloc(sizeof(key_value_pair));
    if(NULL == pair)
    {
        return false;
    }
    pair->key = key;
    pair->value = value;
    ensure_capacity(key_value_pair, mapping->content.mapping.value, mapping->content.size + 1, mapping->content.mapping.capacity);
    mapping->content.mapping.value[mapping->content.size++] = pair;

    return result;
}


