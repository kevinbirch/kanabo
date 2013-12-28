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

#include "model.h"

static inline void sequence_free(node *sequence);
static inline void mapping_free(node *mapping);

static bool sequence_freedom_iterator(void *each, void *context);
static bool mapping_freedom_iterator(void *key, void *value, void *context);

void model_free(document_model *model)
{
    if(NULL == model)
    {
        return;
    }
    vector_iterate(model->documents, sequence_freedom_iterator, NULL);
    vector_free(model->documents);
    model->documents = NULL;
    
    free(model);
}

void node_free(node *value)
{
    if(NULL == value)
    {
        return;
    }
    switch(node_kind(value))
    {
        case DOCUMENT:
            node_free(value->content.target);
            value->content.target = NULL;
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
        case ALIAS:
            break;
    }
    free(value->tag.name);
    free(value->anchor);
    free(value);
}

static bool sequence_freedom_iterator(void *each, void *context __attribute__((unused)))
{
    node_free((node *)each);

    return true;
}
static inline void sequence_free(node *sequence)
{
    if(NULL == sequence->content.sequence)
    {
        return;
    }
    vector_iterate(sequence->content.sequence, sequence_freedom_iterator, NULL);
    vector_free(sequence->content.sequence);
    sequence->content.sequence = NULL;
}

static bool mapping_freedom_iterator(void *key, void *value, void *context __attribute__((unused)))
{
    node_free((node *)key);
    node_free((node *)value);
    
    return true;
}

static inline void mapping_free(node *mapping)
{
    if(NULL == mapping->content.mapping)
    {
        return;
    }
    
    hashtable_iterate(mapping->content.mapping, mapping_freedom_iterator, NULL);
    hashtable_free(mapping->content.mapping);
    mapping->content.mapping = NULL;
}
