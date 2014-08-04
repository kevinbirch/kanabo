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
#include <yaml.h>

#include "emit/yaml.h"
#include "log.h"

static bool emit_nodelist(const nodelist *list, yaml_emitter_t *emitter);
static bool emit_node(node *value, void *context);
static bool emit_document(node *document, void *context);
static bool emit_sequence(node *sequence, void *context);
static bool emit_sequence_item(node *each, void *context);
static bool emit_mapping(node *mapping, void *context);
static bool emit_mapping_item(node *key, node *value, void *context);
static bool emit_scalar(const node *each, void *context);
static bool emit_tagged_scalar(const node *scalar, yaml_char_t *tag, yaml_scalar_style_t style, int implicit, void *context);

#define component "yaml"
#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(LVL_TRACE, component, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)

void emit_yaml(const nodelist *list, const struct options *options)
{
    log_debug(component, "emitting...");
    yaml_emitter_t emitter;
    yaml_event_t event;

    yaml_emitter_initialize(&emitter);
    yaml_emitter_set_output_file(&emitter, stdout);
    yaml_emitter_set_unicode(&emitter, 1);

    log_trace(component, "stream start");
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    if (!yaml_emitter_emit(&emitter, &event))
        goto error;

    log_trace(component, "document start");
    yaml_document_start_event_initialize(&event, &(yaml_version_directive_t){1, 1}, NULL, NULL, 0);
    if (!yaml_emitter_emit(&emitter, &event))
        goto error;

    if(1 == nodelist_length(list))
    {
        if(!emit_node(nodelist_get(list, 0), &emitter))
        {
            fprintf(stderr, "%s: %s\n", options->program_name, emitter.problem);
            goto error;
        }
    }
    else
    {
        if(!emit_nodelist(list, &emitter))
        {
            fprintf(stderr, "%s: %s\n", options->program_name, emitter.problem);
            goto error;
        }
    }

    log_trace(component, "document end");
    yaml_document_end_event_initialize(&event, 1);
    if (!yaml_emitter_emit(&emitter, &event))
        goto error;

    log_trace(component, "stream end");
    yaml_stream_end_event_initialize(&event);
    if (!yaml_emitter_emit(&emitter, &event))
        goto error;

  error:
    yaml_emitter_delete(&emitter);
}

static bool emit_nodelist(const nodelist *list, yaml_emitter_t *emitter)
{
    log_trace(component, "emitting nodelist");
    yaml_event_t event;

    log_trace(component, "seqence start");
    yaml_sequence_start_event_initialize(&event, NULL, (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG, 1, YAML_BLOCK_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!nodelist_iterate(list, emit_sequence_item, emitter))
    {
        return false;
    }

    log_trace(component, "seqence end");
    yaml_sequence_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_node(node *each, void *context)
{
    bool result = true;
    switch(node_kind(each))
    {
        case DOCUMENT:
            result = emit_document(each, context);
            break;
        case SCALAR:
            result = emit_scalar(each, context);
            break;
        case SEQUENCE:
            result = emit_sequence(each, context);
            break;
        case MAPPING:
            result = emit_mapping(each, context);
            break;
        case ALIAS:
            result = emit_node(alias_target(each), context);
            break;
    }

    return result;
}

static bool emit_document(node *document, void *context)
{
    log_trace(component, "emitting document");
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    yaml_document_start_event_initialize(&event, &(yaml_version_directive_t){1, 1}, NULL, NULL, 0);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!emit_node(document_root(document), context))
    {
        return false;
    }

    yaml_document_end_event_initialize(&event, 0);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_sequence(node *sequence, void *context)
{
    log_trace(component, "emitting seqence");
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    log_trace(component, "seqence start");
    uint8_t *name = node_name(sequence);
    yaml_char_t *tag = NULL == name ? (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG : (yaml_char_t *)name;
    yaml_sequence_start_event_initialize(&event, NULL, tag, NULL == name, YAML_BLOCK_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!sequence_iterate(sequence, emit_sequence_item, context))
    {
        return false;
    }

    log_trace(component, "seqence end");
    yaml_sequence_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_sequence_item(node *each, void *context)
{
    log_trace(component, "emitting sequence item");
    return emit_node(each, context);
}

static bool emit_mapping(node *mapping, void *context)
{
    log_trace(component, "emitting mapping");
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    log_trace(component, "mapping start");
    uint8_t *name = node_name(mapping);
    yaml_char_t *tag = NULL == name ? (yaml_char_t *)YAML_DEFAULT_MAPPING_TAG : (yaml_char_t *)name;
    yaml_mapping_start_event_initialize(&event, NULL, tag, NULL == name, YAML_BLOCK_MAPPING_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!mapping_iterate(mapping, emit_mapping_item, context))
    {
        return false;
    }

    log_trace(component, "mapping end");
    yaml_mapping_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_mapping_item(node *key, node *value, void *context)
{
    log_trace(component, "emitting mapping item");
    if(!emit_tagged_scalar(key, (yaml_char_t *)YAML_STR_TAG, YAML_PLAIN_SCALAR_STYLE, 1, context))
    {
        return false;
    }
    return emit_node(value, context);
}

static bool emit_scalar(const node *each, void *context)
{
    yaml_char_t *tag = NULL;
    yaml_scalar_style_t style = YAML_PLAIN_SCALAR_STYLE;

    switch(scalar_kind(each))
    {
        case SCALAR_STRING:
            tag = NULL == node_name(each) ? (yaml_char_t *)YAML_STR_TAG : node_name(each);
            style = YAML_DOUBLE_QUOTED_SCALAR_STYLE;
            break;
        case SCALAR_INTEGER:
            tag = NULL == node_name(each) ? (yaml_char_t *)YAML_INT_TAG : node_name(each);
            break;
        case SCALAR_REAL:
            tag = NULL == node_name(each) ? (yaml_char_t *)YAML_FLOAT_TAG : node_name(each);
            break;
        case SCALAR_TIMESTAMP:
            tag = NULL == node_name(each) ? (yaml_char_t *)YAML_TIMESTAMP_TAG : node_name(each);
            break;
        case SCALAR_BOOLEAN:
            tag = NULL == node_name(each) ? (yaml_char_t *)YAML_BOOL_TAG : node_name(each);
            break;
        case SCALAR_NULL:
            tag = NULL == node_name(each) ? (yaml_char_t *)YAML_NULL_TAG : node_name(each);
            break;
    }

    return emit_tagged_scalar(each, tag, style, NULL == node_name(each), context);
}

static bool emit_tagged_scalar(const node *scalar, yaml_char_t *tag, yaml_scalar_style_t style, int implicit, void *context)
{
    trace_string("emitting scalar \"%s\"", scalar_value(scalar), node_size(scalar));
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    yaml_scalar_event_initialize(&event, NULL, tag, scalar_value(scalar), (int)node_size(scalar), implicit, implicit, style);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}
