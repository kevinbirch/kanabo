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

#include <errno.h>

#include "nodelist.h"
#include "conditions.h"

struct context_adapter_s
{
    union
    {
        nodelist_iterator foreach;
        nodelist_map_function map;
    } iterator;
    void *context;
};

typedef struct context_adapter_s context_adapter;

static bool nodelist_iterator_adpater(void *each, void *context);
static bool nodelist_map_adpater(void *each, void *context, Vector *target);


bool nodelist_set(nodelist *list, void *value, size_t index)
{
    errno = 0;
    vector_set(list, value, index);
    
    return 0 == errno;
}

static bool nodelist_iterator_adpater(void *each, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.foreach((node *)each, adapter->context);
}

bool nodelist_iterate(const nodelist *list, nodelist_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(list, iterator);
    return vector_iterate(list, nodelist_iterator_adpater, &(context_adapter){.iterator.foreach=iterator, context});
}

static bool nodelist_map_adpater(void *each, void *context, Vector *target)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.map((node *)each, adapter->context, target);
}

nodelist *nodelist_map(const nodelist *list, nodelist_map_function function, void *context)
{
    PRECOND_NONNULL_ELSE_NULL(list, function);

    return vector_map(list, nodelist_map_adpater, &(context_adapter){.iterator.map=function, context});
}

nodelist *nodelist_map_into(const nodelist *list, nodelist_map_function function, void *context, nodelist *target)
{
    PRECOND_NONNULL_ELSE_NULL(list, function, target);

    return vector_map_into(list, nodelist_map_adpater, &(context_adapter){.iterator.map=function, context}, target);
}
