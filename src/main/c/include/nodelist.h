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

#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include "model.h"

static const size_t DEFAULT_CAPACITY = 5;

struct nodelist
{
    size_t   length;
    size_t   capacity;
    node   **nodes;
};

typedef struct nodelist nodelist;

nodelist *make_nodelist(void);
nodelist *make_nodelist_with_capacity(size_t capacity);

void nodelist_free(nodelist *value);
void nodelist_free_nodes(nodelist *list);

bool   nodelist_clear(nodelist *list);
size_t nodelist_length(const nodelist * restrict list);
bool   nodelist_is_empty(const nodelist * restrict list);
node  *nodelist_get(const nodelist * restrict list, size_t index);
bool   nodelist_add(nodelist * restrict list,  node * restrict value);
bool   nodelist_add_all(nodelist * restrict list,  nodelist * restrict value);
bool   nodelist_set(nodelist * restrict list, node *value, size_t index);

typedef bool (*nodelist_iterator)(node *each, void *context);
bool nodelist_iterate(const nodelist * restrict list, nodelist_iterator iterator, void *context);

bool add_to_nodelist_sequence_iterator(node *each, void *context);

typedef bool (*nodelist_map_function)(node *each, void *context, nodelist *target);
nodelist *nodelist_map(const nodelist * restrict list, nodelist_map_function function, void *context);
nodelist *nodelist_map_into(const nodelist * restrict list, nodelist_map_function function, void *context, nodelist * restrict target);

