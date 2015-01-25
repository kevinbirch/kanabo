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

#include "model.h"
#include "model/private.h"
#include "conditions.h"


static const char * const SCALAR_KINDS [] =
{
    "string",
    "integer",
    "real",
    "timestamp",
    "boolean",
    "null"
};

const char *scalar_kind_name(const Scalar *self)
{
    return SCALAR_KINDS[scalar_kind(self)];
}

static bool scalar_equals(const Node *one, const Node *two)
{
    size_t n1 = node_size(one);
    size_t n2 = node_size(two);

    if(n1 != n2)
    {
        return false;
    }
    return 0 == memcmp(scalar_value((const Scalar *)one),
                       scalar_value((const Scalar *)two), n1);
}

static size_t scalar_size(const Node *self)
{
    return ((Scalar *)self)->length;
}

static void scalar_free(Node *value)
{
    Scalar *self = (Scalar *)value;
    free(self->value);
    self->value = NULL;
}

static const struct vtable_s scalar_vtable = 
{
    scalar_free,
    scalar_size,
    scalar_equals
};

Scalar *make_scalar_node(const uint8_t *value, size_t length, enum scalar_kind kind)
{
    if(NULL == value && 0 != length)
    {
        errno = EINVAL;
        return NULL;
    }

    Scalar *result = calloc(1, sizeof(Scalar));
    if(NULL != result)
    {
        node_init((Node *)result, SCALAR);
        result->length = length;
        result->kind = kind;
        result->value = (uint8_t *)calloc(1, length);
        if(NULL == result->value)
        {
            free(result);
            result = NULL;
            return NULL;
        }
        memcpy(result->value, value, length);
        result->base.vtable = &scalar_vtable;
    }

    return result;
}

uint8_t *scalar_value(const Scalar *self)
{
    PRECOND_NONNULL_ELSE_NULL(self);

    return self->value;
}

enum scalar_kind scalar_kind(const Scalar *self)
{
    return self->kind;
}

bool scalar_boolean_is_true(const Scalar *self)
{
    return 0 == memcmp("true", scalar_value(self), 4);
}

bool scalar_boolean_is_false(const Scalar *self)
{
    return 0 == memcmp("false", scalar_value(self), 5);
}


