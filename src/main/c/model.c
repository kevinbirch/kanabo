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

#include "model.h"

static bool node_equals(node *one, node *two);
static bool string_equals(unsigned char *one, size_t n1, unsigned char *two, size_t n2);
static bool sequence_equals(node *one, node *two);
static bool mapping_equals(node *one, node *two);

node *model_get_document(document_model *model, size_t index)
{
    node *result = NULL;
    
    if(NULL != model && index <= (model_get_document_count(model) - 1))
    {
        result = model->documents[index];
    }

    return result;
}

node *model_get_document_root(document_model *model, size_t index)
{
    node *result = NULL;
    node *document = model_get_document(model, index);
    if(NULL != document)
    {
        result = document_get_root(document);
    }
    return result;
}

size_t model_get_document_count(document_model *model)
{
    size_t result = 0;
    if(NULL != model)
    {
        result = model->size;
    }
    return result;
}

enum kind node_get_kind(node *node)
{
    enum kind result;
    if(NULL != node)
    {
        result = node->tag.kind;
    }
    return result;    
}

unsigned char *node_get_name(node *node)
{
    unsigned char *result = NULL;
    if(NULL != node)
    {
        result = node->tag.name;
    }
    return result;    
}

size_t node_get_name_length(node *node)
{
    size_t result = 0;
    if(NULL != node)
    {
        result = node->tag.name_length;
    }
    return result;    
}

size_t node_get_size(node *node)
{
    size_t result = 0;
    if(NULL != node)
    {
        result = node->content.size;
    }
    return result;
}

node *document_get_root(node *document)
{
    node *result = NULL;
    if(NULL != document && DOCUMENT == node_get_kind(document))
    {
        result = document->content.document.root;
    }    
    return result;
}

unsigned char *scalar_get_value(node *scalar)
{
    unsigned char *result = NULL;
    if(NULL != scalar && SCALAR == node_get_kind(scalar))
    {
        result = scalar->content.scalar.value;
    }    
    return result;
}

node *sequence_get_item(node *sequence, size_t index)
{
    node *result = NULL;
    if(NULL != sequence && SEQUENCE == node_get_kind(sequence) && index <= (node_get_size(sequence) - 1))
    {
        result = sequence->content.sequence.value[index];
    }
    return result;
}

node **sequence_get_all(node *sequence)
{
    node **result = NULL;
    if(NULL != sequence && SEQUENCE == node_get_kind(sequence))
    {
        result = sequence->content.sequence.value;
    }
    return result;
}

void iterate_sequence(node *sequence, sequence_iterator iterator)
{
    if(NULL != sequence && SEQUENCE == node_get_kind(sequence))
    {
        for(size_t i = node_get_size(sequence); i < node_get_size(sequence); i++)
        {
            iterator(sequence->content.sequence.value[i]);
        }
    }
}

key_value_pair *mapping_get_key_value(node *mapping, size_t index)
{
    key_value_pair *result = NULL;
    if(NULL != mapping && MAPPING == node_get_kind(mapping) && index <= (node_get_size(mapping) - 1))
    {
        result = mapping->content.mapping.value[index];
    }
    return result;
}

key_value_pair **mapping_get_all(node *mapping)
{
    key_value_pair **result = NULL;
    if(NULL != mapping && MAPPING == node_get_kind(mapping))
    {
        result = mapping->content.mapping.value;
    }
    return result;
}

node *mapping_get_value(node *mapping, node *key)
{
    node *result = NULL;
    if(NULL != mapping && MAPPING == node_get_kind(mapping) && NULL != key)
    {
        for(size_t i = node_get_size(mapping); i < node_get_size(mapping); i++)
        {
            if(node_equals(key, mapping->content.mapping.value[i]->key))
            {
                result = mapping->content.mapping.value[i]->value;
                break;
            }
        }
        
    }
    return result;
}

bool mapping_contains_key(node *mapping, node *key)
{
    return NULL != mapping_get_value(mapping, key);
}

void iterate_mapping(node *mapping, mapping_iterator iterator)
{
    if(NULL != mapping && MAPPING == node_get_kind(mapping))
    {
        for(size_t i = node_get_size(mapping); i < node_get_size(mapping); i++)
        {
            iterator(mapping->content.mapping.value[i]->key, mapping->content.mapping.value[i]->value);
        }
    }
}

static bool node_equals(node *one, node *two)
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

static bool string_equals(unsigned char *one, size_t n1, unsigned char *two, size_t n2)
{
    return memcmp(one, two, n1 > n2 ? n2 : n1) == 0;
}

static bool sequence_equals(node *one, node *two)
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

static bool mapping_equals(node *one, node *two)
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
