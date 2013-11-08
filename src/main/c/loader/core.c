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

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>             /* for floor() */
#include <regex.h>

#include "loader.h"
#include "loader/private.h"

typedef bool (*collector)(node *each, void *context);

struct mapping_context
{
    node *key;
    node *mapping;
};

static void event_loop(loader_context *context);
static bool dispatch_event(yaml_event_t *event, loader_context *context);

static bool add_scalar(loader_context *context, yaml_event_t *event);
static enum scalar_kind resolve_scalar_kind(const loader_context *context, const yaml_event_t *event);
static enum scalar_kind tag_to_scalar_kind(const yaml_event_t *tag);
static bool regex_test(const yaml_event_t *event, const regex_t *regex);

static bool add_node(loader_context *context, node *value);

static bool start_mapping(loader_context *context, yaml_char_t *tag);
static bool end_mapping(loader_context *context);
static bool start_sequence(loader_context *context, yaml_char_t *tag);
static bool end_sequence(loader_context *context);
static bool start_document(loader_context *context);
static bool end_document(loader_context *context);


document_model *build_model(loader_context *context)
{
    event_loop(context);

    if(LOADER_SUCCESS == context->code)
    {
        loader_debug("done. found %zd documents.", model_document_count(context->model));
    }
    else
    {
#ifdef USE_LOGGING
        char *message = loader_status_message(context);
        loader_error("aborted. unable to create document model. status: %d (%s)", context->code, message);
        free(message);
#endif
        model_free(context->model);
        context->model = NULL;
    }

    return context->model;
}

static void event_loop(loader_context *context)
{
    yaml_event_t event;    
    memset(&event, 0, sizeof(event));

    loader_trace("entering event loop...");
    context->code = LOADER_SUCCESS;
    while(true)
    {
        if(!yaml_parser_parse(context->parser, &event))
        {
            context->code = interpret_yaml_error(context->parser);
            break;
        }
        if(dispatch_event(&event, context))
        {
            break;
        }
    }
    loader_trace("finished loading");
}

static bool dispatch_event(yaml_event_t *event, loader_context *context)
{
    bool done = false;
    
    switch(event->type)
    {
        case YAML_NO_EVENT:
            loader_trace("received nop event");
            break;
            
        case YAML_STREAM_START_EVENT:
            loader_trace("received stream start event");
            break;
                
        case YAML_STREAM_END_EVENT:
            loader_trace("received stream end event");
            done = true;
            break;
                
        case YAML_DOCUMENT_START_EVENT:
            loader_trace("received document start event");
            done = start_document(context);
            break;

        case YAML_DOCUMENT_END_EVENT:
            loader_trace("received document end event");
            done = end_document(context);
            break;
                
        case YAML_ALIAS_EVENT:
            loader_trace("received alias event");
            break;
                
        case YAML_SCALAR_EVENT:
            loader_trace("received scalar event");
            done = add_scalar(context, event);
            break;                

        case YAML_SEQUENCE_START_EVENT:
            loader_trace("received sequence start event");
            done = start_sequence(context, event->data.sequence_start.tag);
            break;                
                
        case YAML_SEQUENCE_END_EVENT:
            loader_trace("received sequence end event");
            done = end_sequence(context);
            break;
            
        case YAML_MAPPING_START_EVENT:
            loader_trace("received mapping start event");
            done = start_mapping(context, event->data.mapping_start.tag);
            break;

        case YAML_MAPPING_END_EVENT:
            loader_trace("received mapping end event");
            done = end_mapping(context);
            break;                
    }

    return done;
}

static bool add_scalar(loader_context *context, yaml_event_t *event)
{
    enum scalar_kind kind = resolve_scalar_kind(context, event);

    if(NULL == context->key_holder.value && MAPPING == node_kind(context->target))
    {
        trace_string("caching scalar '%s' (%p) as mapping key", event->data.scalar.value, event->data.scalar.length, event->data.scalar.value);
        context->key_holder.value = event->data.scalar.value;
        context->key_holder.length = event->data.scalar.length;
        return false;
    }

    node *scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, kind);
    if(NULL == scalar)
    {
        loader_error("uh oh! couldn't create scalar node, aborting...");
        return true;
    }
    if(NULL != event->data.scalar.tag)
    {
        node_set_tag(scalar, event->data.scalar.tag, strlen((char *)event->data.scalar.tag));
    }

    return add_node(context, scalar);
}

static enum scalar_kind resolve_scalar_kind(const loader_context *context, const yaml_event_t *event)
{
    enum scalar_kind kind = SCALAR_STRING;

    if(NULL != event->data.scalar.tag)
    {
        kind = tag_to_scalar_kind(event);
    }
    else if(YAML_SINGLE_QUOTED_SCALAR_STYLE == event->data.scalar.style ||
            YAML_DOUBLE_QUOTED_SCALAR_STYLE == event->data.scalar.style)
    {
        trace_string("found scalar string '%s', len: %zd", event->data.scalar.value, event->data.scalar.length, event->data.scalar.length);
        kind = SCALAR_STRING;
    }
    else if(0 == memcmp("null", event->data.scalar.value, 4))
    {
        loader_trace("found scalar null");
        kind = SCALAR_NULL;
    }
    else if(0 == memcmp("true", event->data.scalar.value, 4) ||
            0 == memcmp("false", event->data.scalar.value, 5))
    {
        trace_string("found scalar boolean '%s'", event->data.scalar.value, event->data.scalar.length);
        kind = SCALAR_BOOLEAN;
    }
    else if(regex_test(event, context->integer_regex))
    {
        trace_string("found scalar integer '%s'", event->data.scalar.value, event->data.scalar.length);
        kind = SCALAR_INTEGER;
    }
    else if(regex_test(event, context->decimal_regex))
    {
        trace_string("found scalar real '%s'", event->data.scalar.value, event->data.scalar.length);
        kind = SCALAR_REAL;
    }
    else if(regex_test(event, context->timestamp_regex))
    {
        trace_string("found scalar timestamp '%s'", event->data.scalar.value, event->data.scalar.length);
        kind = SCALAR_TIMESTAMP;
    }
    else
    {
        trace_string("defaulting to scalar string '%s'", event->data.scalar.value, event->data.scalar.length, event->data.scalar.length);
    }

    return kind;
}

static enum scalar_kind tag_to_scalar_kind(const yaml_event_t *event)
{
    const yaml_char_t * tag = event->data.scalar.tag;
    if(0 == memcmp(YAML_NULL_TAG, tag, strlen(YAML_NULL_TAG)))
    {
        trace_string("found yaml null tag for scalar '%s'", event->data.scalar.value, event->data.scalar.length);
        return SCALAR_NULL;
    }
    if(0 == memcmp(YAML_BOOL_TAG, tag, strlen(YAML_BOOL_TAG)))
    {
        trace_string("found yaml boolean tag for scalar '%s'", event->data.scalar.value, event->data.scalar.length);
        return SCALAR_BOOLEAN;
    }
    if(0 == memcmp(YAML_STR_TAG, tag, strlen(YAML_STR_TAG)))
    {
        trace_string("found yaml string tag for scalar '%s'", event->data.scalar.value, event->data.scalar.length);
        return SCALAR_STRING;
    }
    if(0 == memcmp(YAML_INT_TAG, tag, strlen(YAML_INT_TAG)))
    {
        trace_string("found yaml integer tag for scalar '%s'", event->data.scalar.value, event->data.scalar.length);
        return SCALAR_INTEGER;
    }
    if(0 == memcmp(YAML_FLOAT_TAG, tag, strlen(YAML_FLOAT_TAG)))
    {
        trace_string("found yaml float tag for scalar '%s'", event->data.scalar.value, event->data.scalar.length);
        return SCALAR_REAL;
    }
    if(0 == memcmp(YAML_TIMESTAMP_TAG, tag, strlen(YAML_TIMESTAMP_TAG)))
    {
        trace_string("found yaml timestamp tag for scalar '%s'", event->data.scalar.value, event->data.scalar.length);
        return SCALAR_TIMESTAMP;
    }
    trace_string("found non-yaml tag for scalar '%s', assuming string", event->data.scalar.value, event->data.scalar.length);
    return SCALAR_STRING;
}

static bool regex_test(const yaml_event_t *event, const regex_t *regex)
{
    char string[event->data.scalar.length + 1];
    memcpy(string, event->data.scalar.value, event->data.scalar.length);
    string[event->data.scalar.length] = '\0';

    return 0 == regexec(regex, string, 0, NULL, 0);
}

static bool add_node(loader_context *context, node *value)
{    
    switch(node_kind(context->target))
    {
        case DOCUMENT:
            loader_trace("adding node to document context (%p)", context->target);
            return !document_set_root(context->target, value);
        case SEQUENCE:
            loader_trace("adding node to sequence context (%p)", context->target);
            return !sequence_add(context->target, value);
        case MAPPING:
        {
            loader_trace("adding node to mapping context (%p)", context->target);
            bool done = !mapping_put(context->target, context->key_holder.value, context->key_holder.length, value);
            if(!done)
            {
                context->key_holder.value = NULL;
                context->key_holder.length = 0ul;
            }
            return done;
        }
        case SCALAR:
            loader_error("uh oh! a scalar node has become the context node, aborting...");
            return true;
    }

    return false;
}


static bool start_document(loader_context *context)
{
    node *document = make_document_node();
    if(NULL == document)
    {
        loader_error("uh oh! couldn't create new document node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    loader_trace("started document (%p)", document);
    context->target = document;
    return false;
}

static bool end_document(loader_context *context)
{
    node *document = context->target;
    loader_trace("completed document (%p)", document);

    model_add(context->model, document);
    loader_trace("added document (%p) to model (%p)", document, context->model);
    context->target = NULL;

    return false;
}
static bool start_sequence(loader_context *context, yaml_char_t *tag)
{
    if(NULL == context->key_holder.value && MAPPING == node_kind(context->target))
    {
        loader_error("uh oh! found a non scalar mapping key, aborting...");
        context->code = ERR_NON_SCALAR_KEY;
        return true;
    }
    node *sequence = make_sequence_node();
    if(NULL == sequence)
    {
        loader_error("uh oh! couldn't create a sequence node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    if(NULL != tag)
    {
        node_set_tag(sequence, tag, strlen((char *)tag));
    }
    loader_trace("started sequence (%p)", sequence);
    
    add_node(context, sequence);

    context->target = sequence;
    return false;
}

static bool end_sequence(loader_context *context)
{
    node *sequence = context->target;
    loader_trace("completed sequence (%p)", sequence);
    loader_trace("added sequence (%p) of length: %zd", sequence, node_size(sequence));
    vector_trim(sequence->content.sequence);
    context->target = sequence->parent;
    
    return false;
}

static bool start_mapping(loader_context *context, yaml_char_t *tag)
{
    if(NULL == context->key_holder.value && MAPPING == node_kind(context->target))
    {
        loader_error("uh oh! found a non scalar mapping key, aborting...");
        context->code = ERR_NON_SCALAR_KEY;
        return true;
    }
    node *mapping = make_mapping_node();
    if(NULL == mapping)
    {
        loader_error("uh oh! couldn't create a mapping node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    if(NULL != tag)
    {
        node_set_tag(mapping, tag, strlen((char *)tag));
    }
    loader_trace("started mapping (%p)", mapping);
    
    add_node(context, mapping);

    context->target = mapping;
    return false;
}

static bool end_mapping(loader_context *context)
{
    node *mapping = context->target;
    loader_trace("completed mapping (%p)", mapping);
    loader_trace("loaded mapping of length: %zd", node_size(mapping));
    context->target = mapping->parent;

    return false;
}
