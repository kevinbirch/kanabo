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
#include <yaml.h>

#include "loader.h"

struct cell
{
    node *this;
    struct cell *next;
};

struct excursion
{
    size_t length;
    struct excursion *next;
};

struct context
{
    struct excursion *excursions;
    
    size_t depth;
    struct cell *top;
    struct cell *stack;
};

typedef struct context document_context;

struct source
{
    enum
    {
        STRING_INPUT,
        FILE_INPUT
    } kind;

    union
    {
        struct
        {
            const unsigned char *string;
            size_t size;
        };
        FILE *file;
    };
};

typedef struct source source;

static loader_result *load_model_from_source(const source * restrict input, document_model * restrict model);
static void prepare_parser_source(yaml_parser_t *parser, const source * restrict input);
static loader_result *build_model(yaml_parser_t *parser, document_model * restrict model, loader_result * restrict result);

static bool dispatch_event(yaml_event_t *event, document_context *context, loader_result * restrict result);

static bool push_context(document_context *context, node *value);
static node *pop_context(document_context *context);
static void abort_context(document_context *context);

static bool save_excursion(document_context *context);
static size_t unwind_excursion(document_context *context);

static bool unwind_sequence(document_context *context);
static bool unwind_mapping(document_context *context);
static bool unwind_document(document_context *context);
static bool unwind_model(document_context *context, document_model *model);

static inline enum kind context_kind(document_context *context);
static inline node *context_top(document_context *context);

static inline loader_result *invalid_input(void);
static inline loader_result *memory_exhausted(loader_result *result);
static inline loader_result *success(loader_result *result);
static loader_result *parser_error(yaml_parser_t *parser, loader_result *result);

loader_result *load_model_from_string(const unsigned char *string, size_t size, document_model * restrict model)
{
    if(NULL == model || NULL == string || 0 == size)
    {        
        return invalid_input();
    }
    
    source input;
    input.kind = STRING_INPUT;
    input.string = string;
    input.size = size;
    
    return load_model_from_source(&input, model);
}

loader_result *load_model_from_file(FILE * restrict file, document_model * restrict model)
{
    if(NULL == model || NULL == file)
    {        
        return invalid_input();
    }
    
    source input;
    input.kind = FILE_INPUT;
    input.file = file;
    
    return load_model_from_source(&input, model);
}

static loader_result *load_model_from_source(const source * restrict input, document_model * restrict model)
{
    memset(model, 0, sizeof(document_model));
    loader_result *result = (loader_result *)malloc(sizeof(loader_result));
    if(NULL == result)
    {
        return NULL;
    }

    yaml_parser_t parser;    
    memset(&parser, 0, sizeof(parser));

    if(!yaml_parser_initialize(&parser))
    {
        parser_error(&parser, result);
        yaml_parser_delete(&parser);
        return result;
    }

    if(!init_model(model, 1))
    {
        yaml_parser_delete(&parser);
        free_model(model);
        return memory_exhausted(result);
    }

    prepare_parser_source(&parser, input);
    result = build_model(&parser, model, result);
    
    yaml_parser_delete(&parser);
    
    return result;
}

static void prepare_parser_source(yaml_parser_t *parser, const source * restrict input)
{
    switch(input->kind)
    {
        case STRING_INPUT:
            yaml_parser_set_input_string(parser, input->string, input->size);
            break;
        case FILE_INPUT:
            yaml_parser_set_input_file(parser, input->file);
            break;
    }
}

static loader_result *build_model(yaml_parser_t *parser, document_model * restrict model, loader_result * restrict result)
{
    yaml_event_t event;
    
    memset(&event, 0, sizeof(event));

    document_context context;
    memset(&context, 0, sizeof(context));
    
    result = success(result);
    bool done = false;
    while(!done)
    {
        if (!yaml_parser_parse(parser, &event))
        {
            result = parser_error(parser, result);
            break;
        }

        done = dispatch_event(&event, &context, result);
    }

    if(SUCCESS == result->code)
    {
        unwind_model(&context, model);
    }
    else
    {
        free_model(model);
        abort_context(&context);
    }

    return result;
}

static bool dispatch_event(yaml_event_t *event, document_context *context, loader_result * restrict result)
{
    bool done = false;
    
    switch(event->type)
    {
        case YAML_NO_EVENT:
            break;
            
        case YAML_STREAM_START_EVENT:
            break;
                
        case YAML_STREAM_END_EVENT:
            done = true;
            break;
                
        case YAML_DOCUMENT_START_EVENT:
            break;

        case YAML_DOCUMENT_END_EVENT:
            if(!unwind_document(context))
            {
                result = memory_exhausted(result);
                done = true;
            }
            break;
                
        case YAML_ALIAS_EVENT:
            break;
                
        case YAML_SCALAR_EVENT:
            if(!push_context(context, make_scalar_node(event->data.scalar.value, event->data.scalar.length)))
            {
                result = memory_exhausted(result);
                done = true;
            }
            break;                

        case YAML_SEQUENCE_START_EVENT:
            if(!save_excursion(context))
            {
                result = memory_exhausted(result);
                done = true;
            }
            break;                
                
        case YAML_SEQUENCE_END_EVENT:
            if(!unwind_sequence(context))
            {
                result = memory_exhausted(result);
                done = true;
            }
            break;
            
        case YAML_MAPPING_START_EVENT:
            if(!save_excursion(context))
            {
                result = memory_exhausted(result);
                done = true;
            }
            break;

        case YAML_MAPPING_END_EVENT:
            if(!unwind_mapping(context))
            {
                result = memory_exhausted(result);
                done = true;
            }
            break;                
    }

    return done;
}

static bool save_excursion(document_context *context)
{
    struct excursion *excursion = malloc(sizeof(struct excursion));
    if(NULL == excursion)
    {
        return false;
    }
    excursion->length = 0;

    if(NULL == context->excursions)
    {
        context->excursions = excursion;
        excursion->next = NULL;
    }
    else
    {
        excursion->next = context->excursions;
        context->excursions = excursion;
    }
    return true;
}

static bool push_context(document_context *context, node *value)
{    
    struct cell *current = (struct cell *)malloc(sizeof(struct cell));
    if(NULL == current)
    {
        return false;
    }
    current->this = value;    

    if(NULL == context->stack)
    {
        context->depth = 0;

        context->stack = current;
        current->next = NULL;
        context->top = current;
    }

    context->depth++;
    if(NULL != context->excursions)
    {
        context->excursions->length++;
    }
        
    current->next = context->top;
    context->top = current;

    return true;
}

static node *pop_context(document_context *context)
{
    if(NULL == context || NULL == context->stack || NULL == context->top)
    {
        return NULL;
    }
    
    struct cell *top = context->top;
    node *result = top->this;
    context->top = top->next;
    context->depth--;

    free(top);
    
    if(NULL == context->top)
    {
        context->stack = NULL;
    }
    
    return result;
}

static void abort_context(document_context *context)
{
    node *top = pop_context(context);
    while(NULL != top)
    {
        free_node(top);
        top = pop_context(context);
    }

    size_t length = unwind_excursion(context);
    while(0 != length)
    {
        length = unwind_excursion(context);
    }
}

static size_t unwind_excursion(document_context *context)
{
    if(NULL == context || NULL == context->excursions)
    {
        return 0;
    }
    
    struct excursion *top = context->excursions;
    size_t result = top->length;
    context->excursions = top->next;

    free(top);
    
    return result;
}

static bool unwind_sequence(document_context *context)
{
    size_t count = unwind_excursion(context);
    node **items = (node **)malloc(sizeof(node *) * count);
    if(NULL == items)
    {
        return false;
    }
    for(size_t i = 0; i < count; i++)
    {
        items[(count - 1) - i] = pop_context(context);
    }

    node *sequence = make_sequence_node(count);
    sequence_add_all(sequence, items, count);
    free(items);
    push_context(context, sequence);
    return true;
}

static bool unwind_mapping(document_context *context)
{
    size_t count = unwind_excursion(context) / 2;
    node *mapping = make_mapping_node(count);
    
    node *key, *value;
    for(size_t i = 0; i < count; i++)
    {
        value = pop_context(context);
        key = pop_context(context);
        mapping_put(mapping, key, value);
    }

    push_context(context, mapping);
    return true;
}

static bool unwind_document(document_context *context)
{
    node *root = pop_context(context);
    node *document = make_document_node(root);
    push_context(context, document);
    return true;
}

static bool unwind_model(document_context *context, document_model *model)
{
    if(1 == context->depth)
    {
        model_add(model, pop_context(context));
    }
    else
    {
        node **documents = (node **)malloc(sizeof(node *) * context->depth);
        if(NULL == documents)
        {
            return false;
        }

        for(size_t i = 0; i < context->depth; i++)
        {
            documents[(context->depth - 1) - i] = pop_context(context);
        }

        for(size_t i = 0; i < context->depth; i++)
        {
            model_add(model, documents[i]);
        }
        free(documents);    
    }
    return true;
}

static inline enum kind context_kind(document_context *context)
{
    return node_get_kind(context_top(context));
}

static inline node *context_top(document_context *context)
{
    return context->top->this;
}

static inline loader_result *invalid_input(void)
{
    loader_result *result = (loader_result *)malloc(sizeof(loader_result));
    if(NULL == result)
    {
        return NULL; // N.B. - memory is exhausted, abort early
    }
    result->code = ERR_INVALID_ARGUMENTS;
    result->dynamic_message = false;
    result->message = "Invalid arguments";
    result->position = 0;
    result->line = 0;

    return result;
}

static inline loader_result *memory_exhausted(loader_result *result)
{
    result->code = ERR_NO_MEMORY;
    result->dynamic_message = false;
    result->message = "Memory is exhausted";
    result->position = 0;
    result->line = 0;

    return result;
}

static inline loader_result *success(loader_result *result)
{
    result->code = SUCCESS;
    result->dynamic_message = false;
    result->message = "Success";
    result->position = 0;
    result->line = 0;

    return result;
}

static loader_result *parser_error(yaml_parser_t *parser, loader_result *result)
{
    switch (parser->error)
    {
        case YAML_MEMORY_ERROR:
            return memory_exhausted(result);
	
        case YAML_READER_ERROR:
            result->code = ERR_READER_FAILED;
            result->position = parser->problem_offset;
            result->line = 0;
            if (parser->problem_value != -1)
            {
                result->dynamic_message = true;
                asprintf(&result->message, "Reader error: %s: #%X at %zd", parser->problem, parser->problem_value, parser->problem_offset);
            }
            else
            {
                result->dynamic_message = true;
                asprintf(&result->message, "Reader error: %s at %zd", parser->problem, parser->problem_offset);
            }
            if(NULL == result->message)
            {
                result->dynamic_message = false;
                result->message = "Reader error";
            }
            break;
	
        case YAML_SCANNER_ERROR:
            result->code = ERR_SCANNER_FAILED;
            result->position = parser->problem_offset;
            result->line = parser->problem_mark.line+1;
            if (parser->context)
            {
                result->dynamic_message = true;
                asprintf(&result->message, "Scanner error: %s at line %ld, column %ld; %s at line %ld, column %ld",
                         parser->context, parser->context_mark.line+1, parser->context_mark.column+1,
                         parser->problem, parser->problem_mark.line+1, parser->problem_mark.column+1);
            }
            else
            {
                result->dynamic_message = true;
                asprintf(&result->message, "Scanner error: %s at line %ld, column %ld",
                         parser->problem, parser->problem_mark.line+1, parser->problem_mark.column+1);
            }
            if(NULL == result->message)
            {
                result->dynamic_message = false;
                result->message = "Scanner error";
            }
            break;
	
        case YAML_PARSER_ERROR:
            result->code = ERR_PARSER_FAILED;
            result->position = parser->problem_offset;
            result->line = parser->problem_mark.line+1;
            if (parser->context)
            {
                result->dynamic_message = true;
                asprintf(&result->message, "Parser error: %s at line %ld, column %ld; %s at line %ld, column %ld",
                         parser->context, parser->context_mark.line+1, parser->context_mark.column+1,
                         parser->problem, parser->problem_mark.line+1, parser->problem_mark.column+1);
            }
            else
            {
                result->dynamic_message = true;
                asprintf(&result->message, "Parser error: %s at line %ld, column %ld",
                         parser->problem, parser->problem_mark.line+1, parser->problem_mark.column+1);
            }
            if(NULL == result->message)
            {
                result->dynamic_message = false;
                result->message = "Parser error";
            }
            break;
	
        default:
            result->code = ERR_OTHER;
            result->position = 0;
            result->line = 0;
            result->dynamic_message = false;
            result->message = "Other error";
            break;
    }

    return result;
}

void free_loader_result(loader_result *result)
{
    if(NULL == result)
    {
        return;
    }
    if(result->dynamic_message)
    {
        free(result->message);
    }
    free(result);
}


