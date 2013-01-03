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
#include "array.h"

bool allocate(nodelist * restrict list, size_t capacity);

nodelist *make_nodelist(void)
{
    return make_nodelist_with_capacity(DEFAULT_CAPACITY);
}

nodelist *make_nodelist_with_capacity(size_t capacity)
{
    errno = 0;
    nodelist *result = (nodelist *)malloc(sizeof(nodelist));
    if(NULL == result)
    {
        return NULL;
    }

    if(allocate(result, capacity))
    {
        return result;
    }
    else
    {
        free(result);
        return NULL;
    }
}

bool allocate(nodelist * restrict list, size_t capacity)
{
    list->length = 0;
    errno = 0;
    list->nodes = (node **)malloc(sizeof(node) * capacity);
    if(NULL == list->nodes)
    {
        return false;
    }
    list->capacity = capacity;
    return true;
}

void nodelist_free(nodelist *list)
{
    if(NULL == list)
    {
        errno = EINVAL;
        return;
    }
    errno = 0;
    if(0 < list->length)
    {
        free(list->nodes);
        list->nodes = NULL;
    }
    free(list);
}

bool nodelist_clear(nodelist *list)
{
    if(NULL == list)
    {
        errno = EINVAL;
        return false;
    }

    errno = 0;
    if(NULL != list->nodes)
    {
        free(list->nodes);
        list->nodes = NULL;
    }
    return allocate(list, DEFAULT_CAPACITY);
}

size_t nodelist_length(const nodelist * restrict list)
{
    if(NULL == list)
    {
        errno = EINVAL;
        return 0;
    }

    errno = 0;
    return list->length;
}

bool nodelist_is_empty(const nodelist * restrict list)
{
    if(NULL == list)
    {
        errno = EINVAL;
        return true;
    }

    errno = 0;
    return 0 == list->length;
}

node *nodelist_get(const nodelist * restrict list, size_t index)
{
    if(NULL == list || index >= list->length)
    {
        errno = EINVAL;
        return false;
    }
    errno = 0;
    return list->nodes[index];
}

bool nodelist_add(nodelist * restrict list,  node *value)
{
    if(NULL == list || NULL == value)
    {
        errno = EINVAL;
        return false;
    }
    ensure_capacity(node, list->nodes, list->length, list->capacity);

    list->nodes[list->length++] = value;
    errno = 0;
    return true;
}

bool nodelist_set(nodelist * restrict list, node *value, size_t index)
{                                                                        
    if(NULL == list || NULL == value || index >= list->length)
    {
        errno = EINVAL;
        return false;
    }

    list->nodes[index] = value;
    errno = 0;
    return true;
}
