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
#include "model/private.h"
#include "conditions.h"
#include "log.h"

static const char * const NODE_KINDS [] =
{
    "document",
    "scalar",
    "sequence",
    "mapping",
    "alias"
};

const char *node_kind_name_(const Node *self)
{
    return NODE_KINDS[node_kind(self)];
}

Node *narrow(Node *instance, NodeKind kind)
{
    if(kind != node_kind(instance))
    {
        log_warn("model", "invalid cast from `%s` to `%s`",
                 node_kind_name(instance), NODE_KINDS[kind]);
    }
    return instance;
}

void node_init_(Node *self, NodeKind kind)
{
    if(NULL != self)
    {
        self->tag.kind = kind;
        self->tag.name = NULL;
        self->anchor = NULL;
    }
}

static void basic_node_free(Node *value)
{
    free(value->tag.name);
    free(value->anchor);
    free(value);
}

void node_free_(Node *value)
{
    if(NULL == value)
    {
        return;
    }
    value->vtable->free(value);
    basic_node_free(value);
}

size_t node_size_(const Node *self)
{
    PRECOND_NONNULL_ELSE_ZERO(self);

    return self->vtable->size(self);
}

NodeKind node_kind_(const Node *self)
{
    return self->tag.kind;
}

uint8_t *node_name_(const Node *self)
{
    PRECOND_NONNULL_ELSE_NULL(self);

    return self->tag.name;
}

Node *node_parent_(const Node *self)
{
    PRECOND_NONNULL_ELSE_NULL(self);
    return self->parent;
}

void node_set_tag_(Node *self, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(self, value);
    self->tag.name = (uint8_t *)calloc(1, length + 1);
    if(NULL != self->tag.name)
    {
        memcpy(self->tag.name, value, length);
        self->tag.name[length] = '\0';
    }
}

void node_set_anchor_(Node *self, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(self, value);
    self->anchor = (uint8_t *)calloc(1, length + 1);
    if(NULL != self->anchor)
    {
        memcpy(self->anchor, value, length);
        self->anchor[length] = '\0';
    }
}

bool node_comparitor(const void *one, const void *two)
{
    return node_equals(one, two);
}

static bool tag_equals(const uint8_t *one, const uint8_t *two)
{
    if(NULL == one && NULL == two)
    {
        return true;
    }
    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }
    size_t n1 = strlen((char *)one);
    size_t n2 = strlen((char *)two);
    return memcmp(one, two, n1 > n2 ? n2 : n1) == 0;
}

bool node_equals_(const Node *one, const Node *two)
{
    if(one == two)
    {
        return true;
    }

    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }

    if(!(node_kind(one) == node_kind(two) &&
       tag_equals(node_name(one), node_name(two))))
    {
        return false;
    }
    if(node_size(one) != node_size(two))
    {
        return false;
    }
    return one->vtable->equals(one, two);
}

