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

enum kind 
{
    DOCUMENT,
    SCALAR,
    SEQUENCE,
    MAPPING
};

struct node
{
    struct 
    {
        enum kind     kind;
        unsigned char *name;
        size_t        name_length;
    } tag;

    struct
    {
        size_t size;
        union
        {
            struct
            {
                unsigned char *value;
            } scalar;
        
            struct
            {
                struct node **value;
            } sequence;
        
            struct
            {
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
    node **documents;
};

typedef struct model document_model;

node  *model_get_document(document_model *model, size_t index);
node  *model_get_document_root(document_model *model, size_t index);
size_t model_get_document_count(document_model *model);

enum kind      node_get_kind(node *node);
unsigned char *get_name(node *node);
size_t         node_get_name_length(node *node);
size_t         node_get_size(node *node);

node *document_get_root(node *document);

unsigned char *scalar_get_value(node *scalar);

node  *sequence_get_item(node *sequence, size_t index);
node **sequence_get_all(node *sequence);

typedef void (*sequence_iterator)(node *each);
void iterate_sequence(node *sequence, sequence_iterator iterator);

key_value_pair  *mapping_get_key_value(node *mapping, size_t index);
key_value_pair **mapping_get_all(node *mapping);
node            *mapping_get_value(node *mapping, node *key);
bool             mapping_contains_key(node *mapping, node *key);

typedef void (*mapping_iterator)(node *key, node *value);
void iterate_mapping(node *mapping, mapping_iterator iterator);

#endif
