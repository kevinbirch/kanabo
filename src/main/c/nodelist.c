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

#define nodelist_ensure_capacity(NODELIST) ensure_capacity(node, (NODELIST)->nodes, (NODELIST)->length, (NODELIST)->capacity)

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
    nodelist_ensure_capacity(list);

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

bool nodelist_iterate(const nodelist * restrict list, nodelist_iterator iterator, void *context)
{
    if(NULL == list || NULL == iterator)
    {
        errno = EINVAL;
        return false;
    }
    for(size_t i = 0; i < nodelist_length(list); i++)
    {
        if(!iterator(list->nodes[i], context))
        {
            return false;
        }
    }
    return true;
}

#include <stdarg.h>

static const void * SENTINEL = (void *)"SENTINEL";

#define ENSURE_NONNULL(ERR_RESULT, ...)                                 \
    if(is_null(__VA_ARGS__, SENTINEL))                                  \
    {                                                                   \
        errno = EINVAL;                                                 \
        return (ERR_RESULT);                                            \
    }

bool is_null(void * first, ...);

bool is_null(void * first, ...)
{
    va_list args;
    bool result = false;
    
    va_start(args, first);
    for(void *arg = first; arg != SENTINEL; arg = va_arg(args, void *))
    {
        if(NULL == arg)
        {
            result = true;
            break;
        }
    }
    va_end(args);
    
    return result;
}

nodelist *nodelist_map(const nodelist * restrict list, nodelist_function function, void *context)
{
    if(NULL == list || NULL == function)
    {
        errno = EINVAL;
        return NULL;
    }
    nodelist *target = make_nodelist_with_capacity(nodelist_length(list));
    if(NULL == target)
    {
        return NULL;
    }
    nodelist *result = nodelist_map_into(list, function, context, target);
    if(NULL == result)
    {
        for(size_t i = 0; i < nodelist_length(target); i++)
        {
            node_free(nodelist_get(target, i));
        }
        nodelist_free(target);
        return result;
    }
    return target;
            
}

nodelist *nodelist_map_into(const nodelist * restrict list, nodelist_function function, void *context, nodelist * restrict target)
{
    if(NULL == list || NULL == function || NULL == target)
    {
        errno = EINVAL;
        return false;
    }
    for(size_t i = 0; i < nodelist_length(list); i++)
    {
        node *each = function(list->nodes[i], context);
        if(NULL == each)
        {
            return NULL;
        }
        if(!nodelist_add(target, each))
        {
            return NULL;
        }
    }

    return target;
}

nodelist *nodelist_map_overwrite(const nodelist * restrict list, nodelist_function function, void *context, nodelist * restrict target)
{
    if(NULL == list || NULL == function || NULL == target || nodelist_length(target) < nodelist_length(list))
    {
        errno = EINVAL;
        return false;
    }
    for(size_t i = 0; i < nodelist_length(list); i++)
    {
        node *each = function(list->nodes[i], context);
        if(NULL == each)
        {
            return NULL;
        }
        if(!nodelist_set(target, each, i))
        {
            return NULL;
        }
    }

    return target;
}

