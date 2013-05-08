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
#include <errno.h>
#include <math.h>             /* for floor() */
#include <stdio.h>            /* for fileno() */
#include <sys/stat.h>         /* for fstat() */

#include "loader.h"
#include "log.h"
#include "conditions.h"

typedef bool (*collector)(node *each, void *context);

struct mapping_context
{
    node *key;
    node *mapping;
};

extern loader_status_code interpret_yaml_error(yaml_parser_t *parser);

static loader_context *make_loader(void);

static document_model *build_model(loader_context *context);
static void event_loop(loader_context *context);
static bool dispatch_event(yaml_event_t *event, loader_context *context);

static bool add_scalar(loader_context *context, yaml_event_t *event);
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
static node *pop_node(loader_context *context, struct item **prev);

#define component_name "loader"

#define loader_info(FORMAT, ...)  log_info(component_name, FORMAT, ##__VA_ARGS__)
#define loader_debug(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define loader_trace(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(TRACE, component_name, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)

loader_context *make_string_loader(const unsigned char *input, size_t size)
{
    loader_debug("creating string loader context");
    loader_context *context = make_loader();
    if(NULL == context || LOADER_SUCCESS != context->code)
    {
        return context;
    }
    if(NULL == input)
    {
        loader_debug("input is null");
        context->code = ERR_INPUT_IS_NULL;
        errno = EINVAL;
        return context;
    }
    if(0 == size)
    {
        loader_debug("input is empty");
        context->code = ERR_INPUT_SIZE_IS_ZERO;
        errno = EINVAL;
        return context;
    }

    yaml_parser_set_input_string(context->parser, input, size);

    return context;
}

loader_context *make_file_loader(FILE * restrict input)
{
    loader_debug("creating file loader context");
    loader_context *context = make_loader();
    if(NULL == context || LOADER_SUCCESS != context->code)
    {
        return context;
    }
    if(NULL == input)
    {
        loader_debug("input is null");
        context->code = ERR_INPUT_IS_NULL;
        errno = EINVAL;
        return context;
    }
    struct stat file_info;
    if(-1 == fstat(fileno(input), &file_info))
    {
        loader_debug("fstat failed on input file");
        context->code = ERR_READER_FAILED;
        errno = EINVAL;
        return context;
    }
    if(feof(input) || 0 == file_info.st_size)
    {
        loader_debug("input is empty");
        context->code = ERR_INPUT_SIZE_IS_ZERO;
        errno = EINVAL;
        return context;
    }

    yaml_parser_set_input_file(context->parser, input);
    return context;
}

static loader_context *make_loader(void)
{
    loader_context *context = (loader_context *)calloc(1, sizeof(loader_context));
    if(NULL == context)
    {
        loader_debug("uh oh! out of memory, can't allocate the loader context");
        return NULL;
    }

    yaml_parser_t *parser = (yaml_parser_t *)calloc(1, sizeof(yaml_parser_t));
    if(NULL == parser)
    {
        loader_debug("uh oh! out of memory, can't allocate the yaml parser");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return context;
    }
    document_model *model = make_model(1);
    if(NULL == model)
    {
        loader_debug("uh oh! out of memory, can't allocate the document model");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return context;
    }
    
    if(!yaml_parser_initialize(parser))
    {
        loader_debug("uh oh! can't initialize the yaml parser");
        context->code = interpret_yaml_error(parser);
        return context;
    }

    context->parser = parser;
    context->model = model;
    context->excursions = NULL;
    context->head = NULL;
    context->last = NULL;

    return context;
}

enum loader_status_code loader_status(const loader_context * restrict context)
{
    return context->code;
}

void loader_free(loader_context *context)
{
    loader_debug("destroying loader context");
    if(NULL == context)
    {
        return;
    }
    yaml_parser_delete(context->parser);
    context->parser = NULL;
    for(struct excursion *entry = context->excursions; NULL != entry; entry = context->excursions)
    {
        context->excursions = entry->next;
        free(entry);
    }
    context->excursions = NULL;
    for(struct item *entry = context->head; NULL != entry; entry = context->head)
    {
        context->head = entry->next;
        free(entry);
    }
    context->head = NULL;
    context->last = NULL;
    context->model = NULL;

    free(context);
}

document_model *load(loader_context *context)
{
    PRECOND_NONNULL_ELSE_NULL(context);
    PRECOND_NONNULL_ELSE_NULL(context->parser);
    PRECOND_NONNULL_ELSE_NULL(context->model);

    loader_debug("starting load...");
    return build_model(context);
}

static document_model *build_model(loader_context *context)
{
    event_loop(context);

    if(LOADER_SUCCESS == context->code)
    {
        unwind_model(context);
        loader_debug("done. found %zd documents.", model_get_document_count(context->model));
    }
    else
    {
#ifdef USE_LOGGING
        char *message = loader_status_message(context);
        loader_debug("aborted. unable to create document model. status: %d (%s)", context->code, message);
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
    for(bool done = false; !done; done = dispatch_event(&event, context))
    {
        if (!yaml_parser_parse(context->parser, &event))
        {
            context->code = interpret_yaml_error(context->parser);
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
        loader_debug("uh oh! couldn't create an excursion object, aborting...");
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
    else
    {
        errno = 0;
        char *endptr;
        strtod((char *)event->data.scalar.value, &endptr);
        if(0 != errno || endptr == (char *)event->data.scalar.value)
        {
            trace_string("found scalar string '%s'", event->data.scalar.value, event->data.scalar.length);
            scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_STRING);
        }
        else
        {
            trace_string("found scalar number '%s'", event->data.scalar.value, event->data.scalar.length);
            scalar = make_scalar_node(event->data.scalar.value, event->data.scalar.length, SCALAR_NUMBER);
        }
    }

    return add_node(context, scalar);
}

static bool add_node(loader_context *context, node *value)
{    
    loader_trace("adding node to cache");
    struct item *current = (struct item *)calloc(1, sizeof(struct item));
    if(NULL == current)
    {
        loader_debug("uh oh! couldn't create a node, aborting...");
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

static bool unwind_excursion(loader_context *loader, collector function, void *context)
{
    loader_trace("unwinding excursion of length: %zd", loader->excursions->length);
    struct item **cursor = NULL == loader->excursions->car ? &loader->head : &loader->excursions->car->next;
    while(NULL != *cursor)
    {
        if(!function(pop_node(loader, cursor), context))
        {
            loader_debug("uh oh! excursion collector failed, aborting...");
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

static bool collect_sequence(node *each, void *context)
{
    node *sequence = (node *)context;
    loader_trace("adding node (%p) to sequence (%p)", each, sequence);
    return sequence_add(sequence, each);
}

static bool unwind_sequence(loader_context *context)
{
    node *sequence = make_sequence_node(context->excursions->length);
    if(NULL == sequence)
    {
        loader_debug("uh oh! couldn't create a sequence node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    loader_trace("unwinding sequence (%p)", sequence);
    if(unwind_excursion(context, collect_sequence, sequence))
    {
        loader_debug("uh oh! couldn't build the sequence, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        free(sequence);
        return true;
    }
    add_node(context, sequence);
    loader_trace("added sequence (%p) of length: %zd", sequence, node_get_size(sequence));
    return false;
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

static bool unwind_mapping(loader_context *context)
{
    node *mapping = make_mapping_node(context->excursions->length / 2);
    if(NULL == mapping)
    {
        loader_debug("uh oh! couldn't create a mapping node, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return true;
    }
    loader_trace("unwinding mapping (%p)", mapping);
    if(unwind_excursion(context, collect_mapping, &(struct mapping_context){.key=NULL, .mapping=mapping}))
    {
        loader_debug("uh oh! couldn't build the mapping, aborting...");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        free(mapping);
        return true;
    }

    add_node(context, mapping);
    loader_trace("added mapping of length: %zd", node_get_size(mapping));
    return false;
}

static bool unwind_document(loader_context *context)
{
    loader_trace("unwinding document");
    node *root = pop_context(context);
    node *document = make_document_node(root);
    if(NULL == document)
    {
        loader_debug("uh oh! couldn't create new document node, aborting...");
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
            loader_debug("uh oh! unable to add document to model, aborting...");
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

static node *pop_context(loader_context *context)
{
    node *result = pop_node(context, &context->head);
    loader_trace("popping node (%p) from head of cache", result);
    return result;
}

static node *pop_node(loader_context *context, struct item **cursor)
{
    node *result = (*cursor)->this;
    loader_trace("extracting node (%p) from cache", result);

    struct item *entry = *cursor;
    *cursor = (*cursor)->next;
    entry->next = NULL;
    free(entry);
    entry = NULL;

    context->length--;

    return result;
}

