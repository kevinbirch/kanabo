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
#include <regex.h>

#include "loader.h"
#include "loader/private.h"

static const char * const DUPLICATE_STRATEGIES [] =
{
    "clobber",
    "warn",
    "fail"
};

static void event_loop(loader_context *context);
static bool dispatch_event(yaml_event_t *event, loader_context *context);

static bool add_scalar(loader_context *context, const yaml_event_t *event);
static ScalarKind resolve_scalar_kind(const loader_context *context, const yaml_event_t *event);
static ScalarKind tag_to_scalar_kind(const yaml_event_t *event);
static inline bool regex_test(const yaml_event_t *event, const regex_t *regex);

static bool cache_mapping_key(loader_context *context, const yaml_event_t *event);
static Scalar *build_scalar_node(loader_context *context, const yaml_event_t *event);

static bool add_alias(loader_context *context, const yaml_event_t *event);

static bool start_document(loader_context *context);
static bool end_document(loader_context *context);
static bool start_sequence(loader_context *context, const yaml_event_t *event);
static bool end_sequence(loader_context *context);
static bool start_mapping(loader_context *context, const yaml_event_t *event);
static bool end_mapping(loader_context *context);

static void set_anchor(loader_context *context, Node *target, uint8_t *anchor);

static bool add_node(loader_context *context, Node *value);
static inline bool add_to_mapping_node(loader_context *context, Node *value);

void build_model(struct loader_context *context)
{
    loader_debug("building model...");
    DocumentModel *model = make_model();
    if(NULL == model)
    {
        loader_error("uh oh! out of memory, can't allocate the document model, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return;
    }
    context->model = model;

    event_loop(context);
    if(LOADER_SUCCESS == context->code && 0 == model_size(context->model))
    {
        loader_error("no documents found for the input!");
        context->code = ERR_NO_DOCUMENTS_FOUND;
        return;
    }
    if(LOADER_SUCCESS == context->code)
    {
        loader_debug("done. found %zd documents.", model_size(context->model));
    }
    else
    {
        model_free(context->model);
        context->model = NULL;
    }
}

static void event_loop(loader_context *context)
{
    yaml_event_t event;
    memset(&event, 0, sizeof(event));

    loader_trace("entering event loop...");
    context->code = LOADER_SUCCESS;
    bool done = false;
    while(!done)
    {
        if(!yaml_parser_parse(&context->parser, &event))
        {
            context->code = interpret_yaml_error(&context->parser);
            break;
        }
        done = dispatch_event(&event, context);
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
            done = add_alias(context, event);
            break;

        case YAML_SCALAR_EVENT:
            loader_trace("received scalar event");
            done = add_scalar(context, event);
            break;

        case YAML_SEQUENCE_START_EVENT:
            loader_trace("received sequence start event");
            done = start_sequence(context, event);
            break;

        case YAML_SEQUENCE_END_EVENT:
            loader_trace("received sequence end event");
            done = end_sequence(context);
            break;

        case YAML_MAPPING_START_EVENT:
            loader_trace("received mapping start event");
            done = start_mapping(context, event);
            break;

        case YAML_MAPPING_END_EVENT:
            loader_trace("received mapping end event");
            done = end_mapping(context);
            break;
    }

    return done;
}

static bool add_scalar(loader_context *context, const yaml_event_t *event)
{

    if(NULL == context->key_holder.value && is_mapping(context->target))
    {
        return cache_mapping_key(context, event);
    }

    Scalar *value = build_scalar_node(context, event);
    if(NULL == value)
    {
        return true;
    }

    loader_trace("added scalar (%p)", value);
    return add_node(context, node(value));
}


static bool cache_mapping_key(loader_context *context, const yaml_event_t *event)
{
    trace_string("caching scalar '%s' (%p) as mapping key", event->data.scalar.value, event->data.scalar.length, event->data.scalar.value);
    context->key_holder.value = event->data.scalar.value;
    context->key_holder.length = event->data.scalar.length;

    if(NULL != event->data.scalar.anchor)
    {
        // this node will be held in the anchor hashtable
        Scalar *key = build_scalar_node(context, event);
        return NULL == key;
    }

    return false;
}

static Scalar *build_scalar_node(loader_context *context, const yaml_event_t *event)
{
    ScalarKind kind = resolve_scalar_kind(context, event);
    Scalar *result = make_scalar_node(event->data.scalar.value, event->data.scalar.length, kind);
    if(NULL == result)
    {
        loader_error("uh oh! couldn't create scalar node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return NULL;
    }
    if(NULL != event->data.scalar.tag)
    {
        node_set_tag(result, event->data.scalar.tag, strlen((char *)event->data.scalar.tag));
    }
    set_anchor(context, node(result), event->data.scalar.anchor);

    return result;
}

static ScalarKind resolve_scalar_kind(const loader_context *context, const yaml_event_t *event)
{
    ScalarKind kind = SCALAR_STRING;

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
    else if(regex_test(event, &context->integer_regex))
    {
        trace_string("found scalar integer '%s'", event->data.scalar.value, event->data.scalar.length);
        kind = SCALAR_INTEGER;
    }
    else if(regex_test(event, &context->decimal_regex))
    {
        trace_string("found scalar real '%s'", event->data.scalar.value, event->data.scalar.length);
        kind = SCALAR_REAL;
    }
    else if(regex_test(event, &context->timestamp_regex))
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

static ScalarKind tag_to_scalar_kind(const yaml_event_t *event)
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

static inline bool regex_test(const yaml_event_t *event, const regex_t *regex)
{
    char string[event->data.scalar.length + 1];
    memcpy(string, event->data.scalar.value, event->data.scalar.length);
    string[event->data.scalar.length] = '\0';

    return 0 == regexec(regex, string, 0, NULL, 0);
}

static bool add_alias(loader_context *context, const yaml_event_t *event)
{
    Node *target = hashtable_get(context->anchors, event->data.alias.anchor);
    if(NULL == target)
    {
        loader_debug("uh oh! couldn't find anchor for alias '%s', aborting...", event->data.alias.anchor);
        context->code = ERR_NO_ANCHOR_FOR_ALIAS;
        return true;
    }

    for(Node *cur = node_parent(context->target); NULL != cur; cur = node_parent(cur))
    {
        if(cur == target)
        {
            loader_debug("uh oh! found an alias loop for '%s', aborting...", event->data.alias.anchor);
            context->code = ERR_ALIAS_LOOP;
            return true;
        }
    }

    loader_trace("added '%s' alias target (%p)", event->data.alias.anchor, target);
    Alias *value = make_alias_node(target);
    return add_node(context, node(value));
}

static bool start_document(loader_context *context)
{
    Document *value = make_document_node();
    if(NULL == value)
    {
        loader_error("uh oh! couldn't create new document node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    loader_trace("started document (%p)", value);
    context->target = node(value);
    return false;
}

static bool end_document(loader_context *context)
{
    loader_trace("completed document (%p)", context->target);

    model_add(context->model, document(context->target));
    loader_trace("added document (%p) to model (%p)", context->target, context->model);
    context->target = NULL;

    return false;
}

static bool start_sequence(loader_context *context, const yaml_event_t *event)
{
    if(NULL == context->key_holder.value && is_mapping(context->target))
    {
        loader_debug("uh oh! found a non scalar mapping key, aborting...");
        context->code = ERR_NON_SCALAR_KEY;
        return true;
    }
    Sequence *seq = make_sequence_node();
    if(NULL == seq)
    {
        loader_error("uh oh! couldn't create a sequence node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    if(NULL != event->data.sequence_start.tag)
    {
        size_t len = strlen((char *)event->data.sequence_start.tag);
        node_set_tag(seq, event->data.sequence_start.tag, len);
    }
    set_anchor(context, node(seq), event->data.sequence_start.anchor);

    loader_trace("started sequence (%p)", seq);

    bool done = add_node(context, node(seq));
    context->target = node(seq);
    return done;
}

static bool end_sequence(loader_context *context)
{
    Node *sequence = context->target;
    loader_trace("completed sequence (%p)", sequence);
    loader_trace("added sequence (%p) of length: %zd", sequence, node_size(sequence));
    vector_trim(sequence(sequence)->values);
    context->target = node_parent(sequence);

    return false;
}

static bool start_mapping(loader_context *context, const yaml_event_t *event)
{
    if(NULL == context->key_holder.value && is_mapping(context->target))
    {
        loader_debug("uh oh! found a non scalar mapping key, aborting...");
        context->code = ERR_NON_SCALAR_KEY;
        return true;
    }
    Mapping *map = make_mapping_node();
    if(NULL == map)
    {
        loader_error("uh oh! couldn't create a mapping node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    if(NULL != event->data.mapping_start.tag)
    {
        size_t length = strlen((char *)event->data.mapping_start.tag);
        node_set_tag(map, event->data.mapping_start.tag, length);
    }
    set_anchor(context, node(map), event->data.mapping_start.anchor);

    loader_trace("started mapping (%p)", map);

    bool done = add_node(context, node(map));
    context->target = node(map);
    return done;
}

static bool end_mapping(loader_context *context)
{
    Node *mapping = context->target;
    loader_trace("completed mapping (%p)", mapping);
    loader_trace("loaded mapping of length: %zd", node_size(mapping));
    context->target = node_parent(mapping);

    return false;
}

static void set_anchor(loader_context *context, Node *target, uint8_t *anchor)
{
    if(NULL == anchor)
    {
        return;
    }
    node_set_anchor(target, anchor, strlen((char *)anchor));

    hashtable_put(context->anchors, anchor, target);
}

static bool add_node(loader_context *context, Node *value)
{
    switch(node_kind(context->target))
    {
        case DOCUMENT:
            loader_trace("adding node (%p) to document context (%p)", value, context->target);
            return !document_set_root(document(context->target), value);
        case SEQUENCE:
            loader_trace("adding node (%p) to sequence context (%p)", value, context->target);
            return !sequence_add(sequence(context->target), value);
        case MAPPING:
            trace_string("adding {\"%s\": node (%p)} to mapping context (%p)", context->key_holder.value, context->key_holder.length, value, context->target);
            return add_to_mapping_node(context, value);
        case SCALAR:
            loader_debug("uh oh! a scalar node has become the context node, aborting...");
            context->code = ERR_OTHER;
            return true;
        case ALIAS:
            loader_debug("uh oh! an alias node has become the context node, aborting...");
            context->code = ERR_OTHER;
            return true;
    }

    return false;
}

static inline bool add_to_mapping_node(loader_context *context, Node *value)
{
    bool duplicate = mapping_contains(mapping(context->target),
                                      context->key_holder.value,
                                      context->key_holder.length);
    if(duplicate && DUPE_FAIL == context->strategy)
    {
        loader_debug("uh oh! a scalar node has become the context node, aborting...");
        context->code = ERR_DUPLICATE_KEY;
        return true;
    }
    if(duplicate && DUPE_WARN == context->strategy)
    {
        char key_name[context->key_holder.length + 1];
        memcpy(key_name, context->key_holder.value, context->key_holder.length);
        key_name[context->key_holder.length] = '\0';
        fprintf(stderr, "warning: duplicate mapping key found: '%s'\n", key_name);
    }
    bool done = !mapping_put(mapping(context->target),
                             context->key_holder.value,
                             context->key_holder.length, value);
    if(!done)
    {
        context->key_holder.value = NULL;
        context->key_holder.length = 0ul;
    }
    return done;
}

int32_t parse_duplicate_strategy(const char *argument)
{
    if(0 == strncmp("clobber", argument, 7ul))
    {
        return DUPE_CLOBBER;
    }
    else if(0 == strncmp("warn", argument, 4ul))
    {
        return DUPE_WARN;
    }
    else if(0 == strncmp("fail", argument, 4ul))
    {
        return DUPE_FAIL;
    }
    else
    {
        return -1;
    }
}

const char * duplicate_strategy_name(enum loader_duplicate_key_strategy value)
{
    return DUPLICATE_STRATEGIES[value];
}
