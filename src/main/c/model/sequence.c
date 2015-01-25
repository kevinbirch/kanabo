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
#include "model/private.h"
#include "conditions.h"


static bool sequence_equals(const Node *one, const Node *two)
{
    return vector_equals(((Sequence *)one)->values,
                         ((Sequence *)two)->values,
                         node_comparitor);
}

static size_t sequence_size(const Node *self)
{
    return vector_length(((Sequence *)self)->values);
}

static bool sequence_freedom_iterator(void *each, void *context __attribute__((unused)))
{
    node_free(node(each));

    return true;
}

static void sequence_free(Node *value)
{
    Sequence *self = (Sequence *)value;
    if(NULL == self->values)
    {
        return;
    }
    vector_iterate(self->values, sequence_freedom_iterator, NULL);
    vector_free(self->values);
    self->values = NULL;
}

static const struct vtable_s sequence_vtable = 
{
    sequence_free,
    sequence_size,
    sequence_equals
};

Sequence *make_sequence_node(void)
{
    Sequence *self = calloc(1, sizeof(Sequence));
    if(NULL != self)
    {
        node_init(node(self), SEQUENCE);
        self->values = make_vector();
        if(NULL == self->values)
        {
            free(self);
            self = NULL;
            return NULL;
        }
        self->base.vtable = &sequence_vtable;
    }

    return self;
}

Node *sequence_get(const Sequence *self, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(self);
    PRECOND_ELSE_NULL(index < vector_length(self->values));

    return vector_get(self->values, index);
}

static bool sequence_iterator_adpater(void *each, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.seq(node(each), adapter->context);
}

bool sequence_iterate(const Sequence *self, sequence_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(self, iterator);

    context_adapter adapter = {.iterator.seq=iterator, .context=context };
    return vector_iterate(self->values, sequence_iterator_adpater, &adapter);
}

bool sequence_add(Sequence *self, Node *item)
{
    PRECOND_NONNULL_ELSE_FALSE(self, item);

    bool result = vector_add(self->values, item);
    if(result)
    {
        item->parent = node(self);
    }
    return result;
}
