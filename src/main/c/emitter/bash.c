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

#include <stdio.h>

#include "emitter/bash.h"
#include "emitter/shell.h"
#include "log.h"

static bool emit_mapping_item(Node *key, Node *value, void *context);

bool emit_bash(const nodelist *list)
{
    log_debug("bash", "emitting %zd items...", nodelist_length(list));
    emit_context context = {
            .emit_mapping_item = emit_mapping_item,
            .wrap_collections = true
    };

    return nodelist_iterate(list, emit_node, &context);
}

static bool emit_mapping_item(Node *key, Node *value, void *context)
{
    if(is_scalar(value))
    {
        log_trace("bash", "emitting mapping item");
        EMIT("[");
        log_trace("bash", "emitting mapping item key");
        if(!emit_raw_scalar(scalar(key)))
        {
            log_error("bash", "uh oh! couldn't emit mapping key");
            return false;
        }
        EMIT("]=");
        log_trace("bash", "emitting mapping item value");
        if(!emit_scalar(scalar(value)))
        {
            log_error("bash", "uh oh! couldn't emit mapping value");
            return false;
        }
        EMIT(" ");
    }
    else
    {
        log_trace("bash", "skipping mapping item");
    }

    return true;
}
