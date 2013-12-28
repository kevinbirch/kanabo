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
#include "conditions.h"

static bool mapping_iterator_adpater(void *key, void *value, void *context);

struct context_adapter_s
{
    mapping_iterator mapping;
    void *context;
};

typedef struct context_adapter_s context_adapter;

node *mapping_get(const node *mapping, uint8_t *scalar, size_t length)
{
    PRECOND_NONNULL_ELSE_NULL(mapping, scalar);
    PRECOND_ELSE_NULL(MAPPING == node_kind(mapping));
    PRECOND_ELSE_NULL(0 < length);

    node *key = make_scalar_node(scalar, length, SCALAR_STRING);
    node *result = hashtable_get(mapping->content.mapping, key);
    node_free(key);
    
    return result;
}

bool mapping_contains(const node *mapping, uint8_t *scalar, size_t length)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, scalar);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping), 0 < length);

    node *key = make_scalar_node(scalar, length, SCALAR_STRING);
    bool result = hashtable_contains(mapping->content.mapping, key);
    node_free(key);
    
    return result;
}

static bool mapping_iterator_adpater(void *key, void *value, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->mapping((node *)key, (node *)value, adapter->context);
}

bool mapping_iterate(const node *mapping, mapping_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, iterator);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping));

    context_adapter adapter = {.mapping=iterator, .context=context};
    return hashtable_iterate(mapping->content.mapping, mapping_iterator_adpater, &adapter);
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

