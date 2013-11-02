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

#define ensure_capacity(TYPE, ARRAY, SIZE, CAPACITY)                    \
    if((SIZE) > (CAPACITY))                                             \
    {                                                                   \
        size_t new_capacity = ((SIZE) * 3) / 2 + 1;                     \
        TYPE **array_cache = (ARRAY);                                   \
        (ARRAY) = realloc((ARRAY), sizeof(TYPE *) * new_capacity);      \
        if(NULL == (ARRAY))                                             \
        {                                                               \
            (ARRAY) = array_cache;                                      \
            return false;                                               \
        }                                                               \
        (CAPACITY) = new_capacity;                                      \
    }

void node_set_tag(node * restrict target, const uint8_t * restrict value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->tag.name = (uint8_t *)calloc(1, length + 1);
    if(NULL != target->tag.name)
    {
        memcpy(target->tag.name, value, length);
        target->tag.name[length] = '\0';
    }
}

void node_set_tag_nocopy(node * restrict target, uint8_t * restrict value)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->tag.name = value;
}

bool model_add(document_model * restrict model, node *document)
{
    PRECOND_NONNULL_ELSE_FALSE(model, document);
    PRECOND_ELSE_FALSE(DOCUMENT == node_kind(document));

    ensure_capacity(node, model->documents, model->size + 1, model->capacity);
    model->documents[model->size++] = document;
    return true;
}

bool document_set_root(node * restrict document, node *root)
{
    PRECOND_NONNULL_ELSE_FALSE(document, root);
    PRECOND_ELSE_FALSE(DOCUMENT == node_kind(document));

    document->content.document.root = root;
    return true;
}

bool sequence_add(node * restrict sequence, node *item)
{
    PRECOND_NONNULL_ELSE_FALSE(sequence, item);
    PRECOND_ELSE_FALSE(SEQUENCE == node_kind(sequence));

    ensure_capacity(node, sequence->content.sequence.value, sequence->content.size + 1, sequence->content.sequence.capacity);
    sequence->content.sequence.value[sequence->content.size++] = item;

    return true;
}

bool sequence_set(node * restrict sequence, node *item, size_t index)
{
    PRECOND_NONNULL_ELSE_FALSE(sequence, item);
    PRECOND_ELSE_FALSE(SEQUENCE == node_kind(sequence), index < sequence->content.size);

    sequence->content.sequence.value[index] = item;

    return true;
}

bool mapping_put(node * restrict mapping, node *key, node *value)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, key, value);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping), SCALAR == node_kind(key));

    errno = 0;
    hashtable_put(mapping->content.mapping, key, value);
    if(0 == errno)
    {
        mapping->content.size++;
    }
    return 0 == errno;
}

