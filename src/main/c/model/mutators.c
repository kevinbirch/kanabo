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

void node_set_tag(node *target, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->tag.name = (uint8_t *)calloc(1, length + 1);
    if(NULL != target->tag.name)
    {
        memcpy(target->tag.name, value, length);
        target->tag.name[length] = '\0';
    }
}

void node_set_anchor(node *target, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->anchor = (uint8_t *)calloc(1, length + 1);
    if(NULL != target->anchor)
    {
        memcpy(target->anchor, value, length);
        target->anchor[length] = '\0';
    }
}

bool model_add(document_model *model, node *document)
{
    PRECOND_NONNULL_ELSE_FALSE(model, document);
    PRECOND_ELSE_FALSE(DOCUMENT == node_kind(document));

    return vector_add(model->documents, document);
}

bool document_set_root(node *document, node *root)
{
    PRECOND_NONNULL_ELSE_FALSE(document, root);
    PRECOND_ELSE_FALSE(DOCUMENT == node_kind(document));

    document->content.target = root;
    root->parent = document;
    return true;
}

bool sequence_add(node *sequence, node *item)
{
    PRECOND_NONNULL_ELSE_FALSE(sequence, item);
    PRECOND_ELSE_FALSE(SEQUENCE == node_kind(sequence));

    bool result = vector_add(sequence->content.sequence, item);
    if(result)
    {
        sequence->content.size = vector_length(sequence->content.sequence);
        item->parent = sequence;
    }
    return result;
}

bool mapping_put(node *mapping, uint8_t *scalar, size_t length, node *value)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, scalar, value);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping));

    node *key = make_scalar_node(scalar, length, SCALAR_STRING);
    if(NULL == key)
    {
        return false;
    }
    errno = 0;
    hashtable_put(mapping->content.mapping, key, value);
    if(0 == errno)
    {
        mapping->content.size = hashtable_size(mapping->content.mapping);
        value->parent = mapping;
    }
    return 0 == errno;
}
