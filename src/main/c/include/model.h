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

#ifndef MODEL_H
#define MODEL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

enum node_kind 
{
    DOCUMENT,
    SCALAR,
    SEQUENCE,
    MAPPING
};

enum scalar_kind
{
    SCALAR_STRING,
    SCALAR_NUMBER,
    SCALAR_BOOLEAN,
    SCALAR_NULL
};

struct node
{
    struct 
    {
        enum node_kind  kind;
        uint8_t        *name;
        size_t          name_length;
    } tag;

    struct
    {
        size_t size;
        union
        {
            struct
            {
                enum scalar_kind kind;
                uint8_t         *value;
            } scalar;
        
            struct
            {
                size_t capacity;
                struct node **value;
            } sequence;
        
            struct
            {
                size_t capacity;
                struct key_value_pair **value;
            } mapping;

            struct
            {
                struct node *root;
            } document;
        };
    } content;
};

typedef struct node node;

struct key_value_pair
{
    node *key;
    node *value;
};

typedef struct key_value_pair key_value_pair;

struct model
{
    size_t size;
    size_t capacity;
    node **documents;
};

typedef struct model document_model;

node   *model_get_document(const document_model * restrict model, size_t index);
node   *model_get_document_root(const document_model * restrict model, size_t index);
size_t  model_get_document_count(const document_model * restrict model);

enum node_kind  node_get_kind(const node * restrict node);
uint8_t        *node_get_name(const node * restrict node);
size_t          node_get_name_length(const node * restrict node);
size_t          node_get_size(const node * restrict node);

node *document_get_root(const node * restrict document);

uint8_t *scalar_get_value(const node * restrict scalar);
enum scalar_kind scalar_get_kind(const node * restrict scalar);
bool scalar_boolean_is_true(const node * restrict scalar);
bool scalar_boolean_is_false(const node * restrict scalar);

node  *sequence_get(const node * restrict sequence, size_t index);
node **sequence_get_all(const node * restrict sequence);

typedef bool (*sequence_iterator)(node *each, void *context);
bool iterate_sequence(const node * restrict sequence, sequence_iterator iterator, void *context);

node            *mapping_get_value(const node * restrict mapping, const char * key);
node            *mapping_get_value_scalar_key(const node * restrict mapping, uint8_t *key, size_t key_length);
node            *mapping_get_value_node_key(const node * restrict mapping, const node *key);
bool             mapping_contains_key(const node * restrict mapping, const char *key);
bool             mapping_contains_node_key(const node * restrict mapping, const node *key);
key_value_pair **mapping_get_all(const node * restrict mapping);

typedef bool (*mapping_iterator)(node *key, node *value, void *context);
bool iterate_mapping(const node * restrict mapping, mapping_iterator iterator, void *context);

document_model *make_model(size_t capacity);
bool            model_init(document_model * restrict model, size_t capacity);

node *make_document_node(node * root);
node *make_sequence_node(size_t capacity);
node *make_mapping_node(size_t capacity);
node *make_scalar_node(const uint8_t *value, size_t length, enum scalar_kind kind);

void model_free(document_model *model);
void node_free(node *value);

bool node_equals(const node *one, const node *two);

bool model_add(document_model * restrict model, node *document);
bool document_set_root(node * restrict document, node *root);
bool sequence_add(node * restrict sequence, node *item);
bool sequence_set(node * restrict sequence, node *item, size_t index);
bool mapping_put(node * restrict mapping, node *key, node *value);

#endif
