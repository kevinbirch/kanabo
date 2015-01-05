/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
' *
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
#include "model/private.h"
#include "conditions.h"


static bool mapping_equals(const Node *one, const Node *two)
{
    return hashtable_equals(((const Mapping *)one)->values,
                            ((const Mapping *)two)->values,
                            node_comparitor);
}

static size_t mapping_size(const Node *self)
{
    return hashtable_size(((const Mapping *)self)->values);
}

static bool mapping_freedom_iterator(void *key, void *value, void *context __attribute__((unused)))
{
    node_free(node(key));
    node_free(node(value));

    return true;
}

static void mapping_free(Node *value)
{
    Mapping *map = (Mapping *)value;
    if(NULL == map->values)
    {
        return;
    }

    hashtable_iterate(map->values, mapping_freedom_iterator, NULL);
    hashtable_free(map->values);
    map->values = NULL;
    basic_node_free(value);
}

static hashcode scalar_hash(const void *key)
{
    const Scalar *value = (const Scalar *)key;
    return shift_add_xor_string_buffer_hash(scalar_value(value), node_size(node(key)));
}

static bool scalar_comparitor(const void *one, const void *two)
{
    return node_equals(const_node(one), const_node(two));
}

static const struct vtable_s mapping_vtable = 
{
    mapping_free,
    mapping_size,
    mapping_equals
};

Mapping *make_mapping_node(void)
{
    Mapping *self = calloc(1, sizeof(Mapping));
    if(NULL != self)
    {
        node_init(node(self), MAPPING);
        self->values = make_hashtable_with_function(scalar_comparitor, scalar_hash);
        if(NULL == self->values)
        {
            free(self);
            self = NULL;
            return NULL;
        }
        self->base.vtable = &mapping_vtable;
    }

    return self;
}

Node *mapping_get(const Mapping *self, uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_NULL(self, value);
    PRECOND_ELSE_NULL(0 < length);

    Scalar *key = make_scalar_node(value, length, SCALAR_STRING);
    Node *result = hashtable_get(self->values, key);
    node_free(node(key));

    return result;
}

bool mapping_contains(const Mapping *self, uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_FALSE(self, value);
    PRECOND_ELSE_FALSE(0 < length);

    Scalar *key = make_scalar_node(value, length, SCALAR_STRING);
    bool result = hashtable_contains(self->values, key);
    node_free(node(key));

    return result;
}

static bool mapping_iterator_adpater(void *key, void *value, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.map(node(key), node(value), adapter->context);
}

bool mapping_iterate(const Mapping *self, mapping_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(self, iterator);

    context_adapter adapter = {.iterator.map=iterator, .context=context};
    return hashtable_iterate(self->values, mapping_iterator_adpater, &adapter);
}

bool mapping_put(Mapping *map, uint8_t *key_name, size_t length, Node *value)
{
    PRECOND_NONNULL_ELSE_FALSE(map, key_name, value);

    Scalar *key = make_scalar_node(key_name, length, SCALAR_STRING);
    if(NULL == key)
    {
        return false;
    }
    errno = 0;
    hashtable_put(map->values, key, value);
    if(0 == errno)
    {
        value->parent = node(map);
    }
    return 0 == errno;
}
