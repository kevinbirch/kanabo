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
static bool regex_test(yaml_event_t *event, regex_t *regex);
static bool add_node(loader_context *context, node *value);

static bool save_excursion(loader_context *context);
static bool unwind_excursion(loader_context *loader, collector function, void *context);

static bool unwind_sequence(loader_context *context);
static bool unwind_mapping(loader_context *context);
static bool unwind_document(loader_context *context);
static bool unwind_model(loader_context *context);

static bool collect_sequence(node *each, void *context);
static bool collect_mapping(node *each, void *context);

static node *pop_context(loader_context *context);
static node *pop_node(loader_context *context, struct cell **prev);

document_model *build_model(loader_context *context)
{
    event_loop(context);

    if(LOADER_SUCCESS == context->code)
    {
        unwind_model(context);
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
    loader_trace("finished parsing");
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
            break;

        case YAML_DOCUMENT_END_EVENT:
            loader_trace("received document end event");
            done = unwind_document(context);
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
            done = save_excursion(context);
            break;                
                
        case YAML_SEQUENCE_END_EVENT:
            loader_trace("received sequence end event");
            done = unwind_sequence(context);
            break;
            
        case YAML_MAPPING_START_EVENT:
            loader_trace("received mapping start event");
            done = save_excursion(context);
            break;

        case YAML_MAPPING_END_EVENT:
            loader_trace("received mapping end event");
            done = unwind_mapping(context);
            break;                
    }

    return done;
}

static bool save_excursion(loader_context *context)
{
    struct excursion *excursion = calloc(1, sizeof(struct excursion));
    if(NULL == excursion)
    {
        loader_error("uh oh! couldn't create an excursion object, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    loader_trace("saving excursion (%p) from node: %p", excursion, NULL == context->last ? NULL : context->last->this);
    excursion->length = 0;
    excursion->car = context->last;

    if(NULL == context->excursions)
    {
        loader_trace("no previous excursion");
        context->excursions = excursion;
        excursion->next = NULL;
    }
    else
    {
        loader_trace("previous excursion (%p) is depth: %zd", context->excursions, context->excursions->length);
        excursion->next = context->excursions;
        context->excursions = excursion;
    }
    return false;
}

static bool add_scalar(loader_context *context, yaml_event_t *event)
{
    node *scalar = NULL;

    if(YAML_SINGLE_QUOTED_SCALAR_STYLE == event->data.scalar.style ||
       YAML_DOUBLE_QUOTED_SCALAR_STYLE == event->data.scalar.style)
    {
        trace_string("found scalar string '%s'", event->data.scalar.value, event->data.scalar.length);
        scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_STRING);
    }
    else if(0 == memcmp("null", event->data.scalar.value, 4))
    {
        loader_trace("found scalar null");
        scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_NULL);
    }
    else if(0 == memcmp("true", event->data.scalar.value, 4) ||
            0 == memcmp("false", event->data.scalar.value, 5))
    {
        trace_string("found scalar boolean '%s'", event->data.scalar.value, event->data.scalar.length);
        scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_BOOLEAN);
    }
    else if(regex_test(event, context->integer_regex))
    {
        trace_string("found scalar integer '%s'", event->data.scalar.value, event->data.scalar.length);
        scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_INTEGER);
    }
    else if(regex_test(event, context->decimal_regex))
    {
        trace_string("found scalar decimal '%s'", event->data.scalar.value, event->data.scalar.length);
        scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_DECIMAL);
    }
    else if(regex_test(event, context->timestamp_regex))
    {
        trace_string("found scalar timestamp '%s'", event->data.scalar.value, event->data.scalar.length);
        scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_TIMESTAMP);
    }
    else
    {
        trace_string("found scalar string '%s'", event->data.scalar.value, event->data.scalar.length);
        scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_STRING);
    }

    return add_node(context, scalar);
}

static bool regex_test(yaml_event_t *event, regex_t *regex)
{
    char string[event->data.scalar.length + 1];
    memcpy(string, event->data.scalar.value, event->data.scalar.length);
    string[event->data.scalar.length] = '\0';

    return 0 == regexec(regex, string, 0, NULL, 0);
}

static bool add_node(loader_context *context, node *value)
{    
    loader_trace("adding node to cache");
    struct cell *current = (struct cell *)calloc(1, sizeof(struct cell));
    if(NULL == current)
    {
        loader_error("uh oh! couldn't create a node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    current->this = value;    
    current->next = NULL;

    if(NULL == context->head)
    {
        loader_trace("cache is empty, this will be the head node");
        context->length = 0;
        context->head = current;
    }
    else
    {
        loader_trace("adding after %p", context->last->this);
        context->last->next = current;
    }
    context->last = current;
    context->length++;

    if(NULL != context->excursions)
    {
        context->excursions->length++;
    }

    loader_trace("added node (%p), cache size: %zd, excursion length: %zd", value, context->length, NULL == context->excursions ? 0 : context->excursions->length);

    return false;
}

static bool unwind_sequence(loader_context *context)
{
    node *sequence = make_sequence_node(context->excursions->length);
    if(NULL == sequence)
    {
        loader_error("uh oh! couldn't create a sequence node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    loader_trace("unwinding sequence (%p)", sequence);
    if(unwind_excursion(context, collect_sequence, sequence))
    {
        loader_error("uh oh! couldn't build the sequence, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        free(sequence);
        return true;
    }
    add_node(context, sequence);
    loader_trace("added sequence (%p) of length: %zd", sequence, node_size(sequence));
    return false;
}

static bool unwind_mapping(loader_context *context)
{
    node *mapping = make_mapping_node(context->excursions->length / 2);
    if(NULL == mapping)
    {
        loader_error("uh oh! couldn't create a mapping node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    loader_trace("unwinding mapping (%p)", mapping);
    if(unwind_excursion(context, collect_mapping, &(struct mapping_context){.key=NULL, .mapping=mapping}))
    {
        loader_error("uh oh! couldn't build the mapping, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        free(mapping);
        return true;
    }

    add_node(context, mapping);
    loader_trace("added mapping of length: %zd", node_size(mapping));
    return false;
}

static bool unwind_document(loader_context *context)
{
    loader_trace("unwinding document");
    node *root = pop_context(context);
    node *document = make_document_node(root);
    if(NULL == document)
    {
        loader_error("uh oh! couldn't create new document node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    add_node(context, document);

    return false;
}

static bool unwind_model(loader_context *context)
{
    loader_trace("unwinding model");
    while(0 < context->length)
    {
        loader_trace("adding document node");
        if(!model_add(context->model, pop_context(context)))
        {
            loader_error("uh oh! unable to add document to model, aborting...");
            context->code = ERR_LOADER_OUT_OF_MEMORY;
            return true;
        }
    }
    return false;
}

/* 
 * Utility Functions
 * =================
 */

static bool collect_sequence(node *each, void *context)
{
    node *sequence = (node *)context;
    loader_trace("adding node (%p) to sequence (%p)", each, sequence);
    return sequence_add(sequence, each);
}

static bool collect_mapping(node *each, void *context)
{
    struct mapping_context *argument = (struct mapping_context *)context;
    if(NULL == argument->key)
    {
        loader_trace("caching mapping key (%p) for next invocation", each);
        argument->key = each;
        return true;
    }
    else
    {
        loader_trace("adding key (%p) and value (%p) to mapping (%p)", argument->key, each, argument->mapping);
        bool result = mapping_put(argument->mapping, argument->key, each);
        argument->key = NULL;
        return result;
    }
}

static bool unwind_excursion(loader_context *loader, collector function, void *context)
{
    loader_trace("unwinding excursion of length: %zd", loader->excursions->length);
    struct cell **cursor = NULL == loader->excursions->car ? &loader->head : &loader->excursions->car->next;
    while(NULL != *cursor)
    {
        if(!function(pop_node(loader, cursor), context))
        {
            loader_error("uh oh! excursion collector failed, aborting...");
            return true;
        }
    }

    loader->last = loader->excursions->car;
    struct excursion *top = loader->excursions;
    loader->excursions = loader->excursions->next;
    free(top);
    loader_trace("cache size now: %zd", loader->length);

    return false;
}

static node *pop_context(loader_context *context)
{
    node *result = pop_node(context, &context->head);
    loader_trace("popping node (%p) from head of cache", result);
    return result;
}

static node *pop_node(loader_context *context, struct cell **cursor)
{
    node *result = (*cursor)->this;
    loader_trace("extracting node (%p) from cache", result);

    struct cell *entry = *cursor;
    *cursor = (*cursor)->next;
    entry->next = NULL;
    free(entry);
    entry = NULL;

    context->length--;

    return result;
}

