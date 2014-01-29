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

#include "jsonpath/model.h"
#include "conditions.h"

static const char * const PATH_KIND_NAMES[] =
{
    "absolute path",
    "relative path"
};

struct context_adapter
{
    void *original_context;
    path_iterator iterator;
};

static bool iterator_adapter(void *each, void *context);


enum path_kind path_kind(const jsonpath *path)
{
    return path->kind;
}

size_t path_length(const jsonpath *path)
{
    PRECOND_NONNULL_ELSE_ZERO(path);
    return path->length;
}

step *path_get(const jsonpath *path, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(path);
    PRECOND_NONNULL_ELSE_NULL(path->steps);
    PRECOND_ELSE_NULL(0 != path->length, index < path->length);
    return vector_get(path->steps, index);
}

static bool iterator_adapter(void *each, void *context)
{
    struct context_adapter *adapter = (struct context_adapter *)context;
    return adapter->iterator((step *)each, adapter->original_context);
}

bool inline path_iterate(const jsonpath *path, path_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(path, iterator);

    return vector_iterate(path->steps, iterator_adapter, &(struct context_adapter){context, iterator});
}

const char *path_kind_name(enum path_kind value)
{
    return PATH_KIND_NAMES[value];
}

