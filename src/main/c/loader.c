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

struct input_holder
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

typedef struct input_holder input_holder;

int build_model_from_input(const input_holder * restrict holder, document_model * restrict model);
void prepare_parser_input(yaml_parser_t *parser, const input_holder * restrict holder);
int build_model(yaml_parser_t *parser, document_model * restrict model);

static inline bool dispatch_event(yaml_event_t *event, document_context *context);

static inline void push_context(document_context *context, node *value);
static inline node *pop_context(document_context *context);

static inline void save_excursion(document_context *context);
static inline size_t unwind_excursion(document_context *context);

static inline void unwind_sequence(document_context *context);
static inline void unwind_mapping(document_context *context);
static inline void unwind_document(document_context *context);
static inline void unwind_model(document_context *context, document_model *model);

static inline enum kind context_kind(document_context *context);
static inline node *context_top(document_context *context);

static inline void set_node_name(node *lvalue, unsigned char *name);

void parser_error(yaml_parser_t *parser);

int build_model_from_string(const unsigned char *input, size_t size, document_model * restrict model)
{
    input_holder holder;
    holder.kind = STRING_INPUT;
    holder.string = input;
    holder.size = size;
    
    return build_model_from_input(&holder, model);
}

int build_model_from_file(FILE * restrict input, document_model * restrict model)
{
    input_holder holder;
    holder.kind = FILE_INPUT;
    holder.file = input;
    
    return build_model_from_input(&holder, model);
}

int build_model_from_input(const input_holder * restrict holder, document_model * restrict model)
{
    if(NULL == model)
    {
        return -1;
    }
    
    int result = 0;
    yaml_parser_t parser;
    
    memset(&parser, 0, sizeof(parser));
    memset(model, 0, sizeof(document_model));

    if (!yaml_parser_initialize(&parser))
    {
        parser_error(&parser);
        result = parser.error;
    }
    else
    {
        prepare_parser_input(&parser, holder);
        result = build_model(&parser, model);
    }
    
    yaml_parser_delete(&parser);
    
    return result;
}

void prepare_parser_input(yaml_parser_t *parser, const input_holder * restrict holder)
{
    switch(holder->kind)
    {
        case STRING_INPUT:
            yaml_parser_set_input_string(parser, holder->string, holder->size);
            break;
        case FILE_INPUT:
            yaml_parser_set_input_file(parser, holder->file);
            break;
    }
}

int build_model(yaml_parser_t *parser, document_model * restrict model)
{
    int result = 0;
    yaml_event_t event;
    
    memset(&event, 0, sizeof(event));

    document_context context;
    memset(&context, 0, sizeof(context));
    
    bool done = false;
    while(!done)
    {
        if (!yaml_parser_parse(parser, &event))
        {
            parser_error(parser);
            result = parser->error;
            break;
        }

        done = dispatch_event(&event, &context);        
    }

    unwind_model(&context, model);
    
    return result;
}

static inline bool dispatch_event(yaml_event_t *event, document_context *context)
{
    bool result = false;
    
    switch(event->type)
    {
        case YAML_NO_EVENT:
            break;
            
        case YAML_STREAM_START_EVENT:
            break;
                
        case YAML_STREAM_END_EVENT:
            result = true;
            break;
                
        case YAML_DOCUMENT_START_EVENT:
            break;

        case YAML_DOCUMENT_END_EVENT:
            unwind_document(context);
            break;
                
        case YAML_ALIAS_EVENT:
            break;
                
        case YAML_SCALAR_EVENT:
            push_context(context, make_scalar_node(event->data.scalar.value, event->data.scalar.length));
            break;                

        case YAML_SEQUENCE_START_EVENT:
            save_excursion(context);
            break;                
                
        case YAML_SEQUENCE_END_EVENT:
            unwind_sequence(context);
            break;
            
        case YAML_MAPPING_START_EVENT:
            save_excursion(context);
            break;

        case YAML_MAPPING_END_EVENT:
            unwind_mapping(context);
            break;                
    }

    return result;
}

static inline void save_excursion(document_context *context)
{
    struct excursion *excursion = malloc(sizeof(struct excursion));
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
}

static inline size_t unwind_excursion(document_context *context)
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

static inline void push_context(document_context *context, node *value)
{    
    struct cell *current = (struct cell *)malloc(sizeof(struct cell));
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
}

static inline node *pop_context(document_context *context)
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

static inline void unwind_sequence(document_context *context)
{
    size_t count = unwind_excursion(context);
    node **items = (node **)malloc(sizeof(node *) * count);
    
    for(size_t i = 0; i < count; i++)
    {
        items[(count - 1) - i] = pop_context(context);
    }

    node *sequence = make_sequence_node(count);
    sequence_add_all(sequence, items, count);
    free(items);
    push_context(context, sequence);
}

static inline void unwind_mapping(document_context *context)
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
}

static inline void unwind_document(document_context *context)
{
    node *root = pop_context(context);
    node *document = make_document_node(root);
    push_context(context, document);
}

static inline void unwind_model(document_context *context, document_model *model)
{
    model->size = context->depth;
    model->documents = (node **)malloc(sizeof(node *) * model->size);
    
    for(size_t i = 0; i < model->size; i++)
    {
        model->documents[(model->size - 1) - i] = pop_context(context);
    }
}

static inline enum kind context_kind(document_context *context)
{
    return context_top(context)->tag.kind;
}

static inline node *context_top(document_context *context)
{
    return context->top->this;
}

static inline void set_node_name(node *lvalue, unsigned char *name)
{
    lvalue->tag.name = name;
}

void parser_error(yaml_parser_t *parser)
{
    switch (parser->error)
    {
        case YAML_MEMORY_ERROR:
            fprintf(stderr, "Memory error: Not enough memory for parsing\n");
            break;
	
        case YAML_READER_ERROR:
            if (parser->problem_value != -1)
            {
                fprintf(stderr, "Reader error: %s: #%X at %zd\n", parser->problem, parser->problem_value, parser->problem_offset);
            }
            else
            {
                fprintf(stderr, "Reader error: %s at %zd\n", parser->problem, parser->problem_offset);
            }
            break;
	
        case YAML_SCANNER_ERROR:
            if (parser->context)
            {
                fprintf(stderr, "Scanner error: %s at line %ld, column %ld\n"
                        "%s at line %ld, column %ld\n", parser->context,
                        parser->context_mark.line+1, parser->context_mark.column+1,
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            else
            {
                fprintf(stderr, "Scanner error: %s at line %ld, column %ld\n",
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            break;
	
        case YAML_PARSER_ERROR:
            if (parser->context)
            {
                fprintf(stderr, "Parser error: %s at line %ld, column %ld\n"
                        "%s at line %ld, column %ld\n", parser->context,
                        parser->context_mark.line+1, parser->context_mark.column+1,
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            else
            {
                fprintf(stderr, "Parser error: %s at line %ld, column %ld\n",
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            break;
	
        default:
            /* Couldn't happen. */
            fprintf(stderr, "Internal error\n");
            break;
    }
}
