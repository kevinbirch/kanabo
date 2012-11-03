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
#include <stdio.h>
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

static inline node *make_document_node();
static inline node *make_scalar_node(size_t length, unsigned char *value);
static inline node *make_sequence_node();
static inline node *make_mapping_node();
static inline node *make_node(enum kind kind);
static inline void set_node_name(node *lvalue, unsigned char *name);

void parser_error(yaml_parser_t *parser);

int load_file(FILE * restrict istream, document_model * restrict model)
{
    if(NULL == model)
    {
        return -1;
    }
    
    int result = 0;
    yaml_parser_t parser;
    
    memset(&parser, 0, sizeof(parser));
    memset(model, 0, sizeof(*model));

    if (!yaml_parser_initialize(&parser))
    {
        parser_error(&parser);
        result = parser.error;
        yaml_parser_delete(&parser);
        return result;
    }

    yaml_parser_set_input_file(&parser, istream);
    result = build_model(&parser, model);
    
    yaml_parser_delete(&parser);
    
    return result;
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
    
    // xxx - should this return a result object instead of an int?
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
            push_context(context, make_document_node());
            break;

        case YAML_DOCUMENT_END_EVENT:
            unwind_document(context);
            break;
                
        case YAML_ALIAS_EVENT:
            // xxx - gather and process anchors and aliases!!!
            break;
                
        case YAML_SCALAR_EVENT:
            // xxx - gather tag and anchor
            push_context(context, make_scalar_node(event->data.scalar.length, event->data.scalar.value));
            break;                

        case YAML_SEQUENCE_START_EVENT:
            // xxx - gather tag and anchor
            push_context(context, make_sequence_node());
            save_excursion(context);
            break;                
                
        case YAML_SEQUENCE_END_EVENT:
            unwind_sequence(context);
            break;
            
        case YAML_MAPPING_START_EVENT:
            // xxx - gather tag and anchor
            push_context(context, make_mapping_node());
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
    // xxx - check for malloc error
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
    // xxx - check for malloc error
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
    node **sequence = (node **)malloc(sizeof(node *) * count);
    // xxx - check for malloc error
    
    for(long i = count - 1; i >= 0; i--)
    {
        sequence[i] = pop_context(context);
    }

    context_top(context)->content.size = count;
    context_top(context)->content.sequence.value = sequence;
}

static inline void unwind_mapping(document_context *context)
{
    size_t count = unwind_excursion(context) / 2;
    struct key_value_pair **mapping = (struct key_value_pair **)malloc(sizeof(struct key_value_pair *) * count);
    // xxx - check for malloc error
    
    struct key_value_pair *each;
    for(size_t i = 0; i < count; i++)
    {
        each = (struct key_value_pair *)malloc(sizeof(struct key_value_pair));
        // xxx - check for malloc error
        
        each->value = pop_context(context);
        each->key = pop_context(context);
    }

    context_top(context)->content.size = count;
    context_top(context)->content.mapping.value = mapping;
}

static inline void unwind_document(document_context *context)
{
    node *value = pop_context(context);
    context_top(context)->content.document.root = value;
}

static inline void unwind_model(document_context *context, document_model *model)
{
    model->size = context->depth;
    model->documents = (node **)malloc(sizeof(node *) * model->size);
    // xxx - check for malloc error
    
    for(long i = model->size - 1; i >= 0; i--)
    {
        model->documents[i] = pop_context(context);
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

static inline node *make_document_node()
{
    node *result = make_node(DOCUMENT);
    result->content.size = 1;
    
    return result;
}

static inline node *make_scalar_node(size_t length, unsigned char *value)
{
    node *result = make_node(SCALAR);
    result->content.size = length;
    result->content.scalar.value = value;
    
    return result;
}

static inline node *make_sequence_node()
{
    node *result = make_node(SEQUENCE);
    result->content.size = 0;
    result->content.sequence.value = NULL;
    
    return result;
}    

static inline node *make_mapping_node()
{
    node *result = make_node(MAPPING);
    result->content.size = 0;
    result->content.mapping.value = NULL;
    
    return result;
}    

static inline node *make_node(enum kind kind)
{
    node *result = (node *)malloc(sizeof(struct node));
    // xxx - check for malloc error
    result->tag.kind = kind;
    
    return result;
}

static inline void set_node_name(node *lvalue, unsigned char *name)
{
    lvalue->tag.name = name;
}

void parser_error(yaml_parser_t *parser)
{
    // xxx - add support for quiet mode
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
