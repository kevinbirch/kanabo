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

