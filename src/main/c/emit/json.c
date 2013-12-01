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

#include "emit/json.h"
#include "log.h"

static bool emit_json_node(node *value, void *context);
static bool emit_json_scalar(const node *each);
static bool emit_json_quoted_scalar(const node *each);
static bool emit_json_raw_scalar(const node *each);
static bool emit_json_sequence_item(node *each, void *context);
static bool emit_json_mapping_item(node *key, node *value, void *context);

#define component "json"

#define EMIT(STR) if(-1 == fprintf(stdout, (STR)))                      \
    {                                                                   \
        log_error(component, "uh oh! couldn't emit literal %s", (STR));    \
        return false;                                                   \
    }    

#define QEMIT(STR) if(-1 == fprintf(stdout, (STR)))                     \
    {                                                                   \
        log_error(component, "uh oh! couldn't emit literal %s", (STR));    \
    }    

void emit_json(const nodelist *list, const struct settings *settings)
{
    log_debug(component, "emitting...");
    size_t count = 0;
    QEMIT("[");
    if(!nodelist_iterate(list, emit_json_sequence_item, &count))
    {
        perror(settings->program_name);
    }
    QEMIT("]");
    QEMIT("\n");
    fflush(stdout);
}

static bool emit_json_node(node *each, void *context __attribute__((unused)))
{
    bool result = true;
    size_t sequence_count = 0;
    size_t mapping_count = 0;
    switch(node_kind(each))
    {
        case DOCUMENT:
            log_trace(component, "emitting document");
            result = emit_json_node(document_root(each), context);
            break;
        case SCALAR:
            result = emit_json_scalar(each);
            break;
        case SEQUENCE:
            log_trace(component, "emitting seqence");
            EMIT("[");
            result = sequence_iterate(each, emit_json_sequence_item, &sequence_count);
            EMIT("]");
            break;
        case MAPPING:
            log_trace(component, "emitting mapping");
            EMIT("{");
            result = mapping_iterate(each, emit_json_mapping_item, &mapping_count);
            EMIT("}");
            break;
        case ALIAS:
            log_trace(component, "resolving alias");
            result = emit_json_node(alias_target(each), context);
            break;
    }

    return result;
}

static bool emit_json_scalar(const node *each)
{
    if(SCALAR_STRING == scalar_kind(each) ||
       SCALAR_TIMESTAMP == scalar_kind(each))
    {
        log_trace(component, "emitting quoted scalar");
        return emit_json_quoted_scalar(each);
    }
    else
    {
        log_trace(component, "emitting raw scalar");
        return emit_json_raw_scalar(each);
    }
}

static bool emit_json_quoted_scalar(const node *each)
{
    EMIT("\"");
    if(!emit_json_raw_scalar(each))
    {
        log_error(component, "uh oh! couldn't emit quoted scalar");
        return false;
    }
    EMIT("\"");

    return true;
}

static bool emit_json_raw_scalar(const node *each)
{
    return fwrite(scalar_value(each), node_size(each), 1, stdout);
}

static bool emit_json_sequence_item(node *each, void *context)
{
    log_trace(component, "emitting sequence item");
    size_t *count = (size_t *)context;
    if(0 != (*count)++)
    {
        EMIT(",");
    }
    return emit_json_node(each, NULL);
}

static bool emit_json_mapping_item(node *key, node *value, void *context)
{
    log_trace(component, "emitting mapping item");
    size_t *count = (size_t *)context;
    if(0 != (*count)++)
    {
        EMIT(",");
    }
    if(!emit_json_quoted_scalar(key))
    {
        return false;
    }
    EMIT(":");
    return emit_json_node(value, NULL);
}

