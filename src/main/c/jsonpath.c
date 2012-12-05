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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "jsonpath.h"

struct node
{
    step *step;
    struct node *next;
};

typedef struct node node;

static const char * const STATES[] =
{
    "start",
    "absoute path",
    "qualified path",
    "relative path",
    "abbreviated relative path",
    "step",
    "name test",
    "wildcard name test",
    "qname test",
    "type test",
    "object type test",
    "array type test",
    "string type test",
    "number type test",
    "boolean type test",
    "null type test",
    "predicate",
    "wildcard",
    "subscript",
    "slice",
    "join",
    "filter",
    "script"
};

enum state
{
    ST_START = 0,
    ST_ABSOLUTE_PATH,
    ST_QUALIFIED_PATH,
    ST_RELATIVE_PATH,
    ST_ABBREVIATED_RELATIVE_PATH,
    ST_STEP,
    ST_NAME_TEST,
    ST_WILDCARD_NAME_TEST,
    ST_QNAME_TEST,
    ST_TYPE_TEST,
    ST_JSON_OBJECT_TYPE_TEST,
    ST_JSON_ARRAY_TYPE_TEST,
    ST_JSON_STRING_TYPE_TEST,
    ST_JSON_NUMBER_TYPE_TEST,
    ST_JSON_BOOLEAN_TYPE_TEST,
    ST_JSON_NULL_TYPE_TEST,
    ST_PREDICATE,
    ST_WILDCARD_PREDICATE,
    ST_SUBSCRIPT_PREDICATE,
    ST_SLICE_PREDICATE,
    ST_JOIN_PREDICATE,
    ST_FILTER_PREDICATE,
    ST_SCRIPT_PREDICATE
};

struct context
{
    uint8_t *input;
    size_t  length;
    size_t  cursor;

    uint8_t expected;
    
    jsonpath *model;
    node *steps;

    enum state state;
    
    jsonpath_status_code code;
};

typedef struct context parser_context;

static const char * const MESSAGES[] = 
{
    "Success",
    "Expression is NULL",
    "Expression length is 0",
    "Output path is NULL",
    "Unable to allocate memory",
    "Not a JSONPath expression",
    "Premature end of input at position %d",
    "At position %d: unexpected char %c, was expecting %c instead",
    "At position %d: expected an integer"
};

static void free_step(step *step);
static void free_predicate(predicate *predicate);

//typedef void (*parser)(parser_context *);
//typedef void *(*combinator)(parser_result *(*first)(, ...);

// combinators
//void *one(parser only);
//void *concat(combinator one, ...);
//void *choice(combinator one, ...);
//void *optional(combinator one, ...);

// production parsers
static void path(parser_context *context);
static void absolute_path(parser_context *context);
static void qualified_path(parser_context *context);
static void relative_path(parser_context *context);

// terminal parsers
/* static void string(parser_context *context); */
/* static void integer(parser_context *context); */
/* static void string_literal(parser_context *context, char *value); */

static inline bool has_more_input(parser_context *context);
static inline uint8_t get_char(parser_context *context);
static inline void consume_char(parser_context *context);

static step *make_root_step(void);
static inline step *make_step(enum kind kind);

static bool push_step(parser_context *context, step *step);
static step *pop_step(parser_context *context);

static inline void unexpected_value(parser_context *context, uint8_t expected);

static inline void enter_state(parser_context *context, enum state state);

static void unwind_context(parser_context *context);
static void abort_context(parser_context *context);

static inline void prepare_context(parser_context *context, uint8_t *expression, size_t length, jsonpath *path);
static inline bool validate(parser_result *result, uint8_t *expression, size_t length, jsonpath *model);

static char *prepare_message(parser_context *context);
static char *prepare_simple_message(jsonpath_status_code code);

void free_jsonpath(jsonpath *model)
{
    if(NULL == model || NULL == model->steps || 0 == model->length)
    {
        return;
    }
    for(size_t i = 0; i < model->length; i++)
    {
        free_step(model->steps[i]);
    }
    free(model->steps);
}

static void free_step(step *step)
{
    if(NULL == step)
    {
        return;
    }
    if(NAME_TEST == step->test.kind)
    {
        if(NULL != step->test.name.prefix)
        {
            free(step->test.name.prefix);
            step->test.name.prefix = NULL;
        }
        if(NULL != step->test.name.local)
        {
            free(step->test.name.local);
            step->test.name.local = NULL;
        }
    }
    if(NULL != step->predicates && 0 != step->predicate_count)
    {
        for(size_t i = 0; i < step->predicate_count; i++)
        {
            free_predicate(step->predicates[i]);
        }
        free(step->predicates);
    }
    free(step);
}

static void free_predicate(predicate *predicate)
{
    if(NULL == predicate)
    {
        return;
    }
    // xxx - need to complete this!!!
    free(predicate);
}

void free_parser_result(parser_result *result)
{
    if(NULL != result)
    {
        if(NULL != result->message)
        {
            free(result->message);
        }
        free(result);
    }
}

parser_result *parse_jsonpath(uint8_t *expression, size_t length, jsonpath *model)
{
    parser_result *result = (parser_result *)malloc(sizeof(parser_result));
    if(NULL == result)
    {
        return NULL;
    }
    result->position = 0;
    if(!validate(result, expression, length, model))
    {
        return result;
    }

    parser_context context;
    prepare_context(&context, expression, length, model);
    
    path(&context);
    if(SUCCESS != context.code)
    {
        abort_context(&context);
    }
    else
    {
        unwind_context(&context);
    }

    result->code = context.code;
    result->message = prepare_message(&context);
    result->position = context.cursor;
    
    return result;
}

static void abort_context(parser_context *context)
{
    if(NULL == context->model->steps)
    {
        return;
    }
    for(size_t i = 0; i < context->model->length; i++)
    {
        free_step(pop_step(context));
    }
    
    context->model->steps = NULL;
    context->model->length = 0;
}

static void unwind_context(parser_context *context)
{
    context->model->steps = (step **)malloc(sizeof(step *) * context->model->length);
    if(NULL == context->model->steps)
    {
        context->code = ERR_OUT_OF_MEMORY;
        abort_context(context);
        return;
    }

    for(size_t i = 0; i < context->model->length; i++)
    {
        context->model->steps[(context->model->length - 1) - i] = pop_step(context);
    }
}

static void path(parser_context *context)
{
    enter_state(context, ST_START);
    absolute_path(context);
    if(SUCCESS == context->code)
    {
        return;
    }

    enter_state(context, ST_START);
    relative_path(context);
}

static void absolute_path(parser_context *context)
{
    enter_state(context, ST_ABSOLUTE_PATH);
    if('$' == get_char(context))
    {
        context->code = SUCCESS;
        context->model->kind = ABSOLUTE_PATH;
        consume_char(context);

        step *root = make_root_step();
        if(NULL == root)
        {
            context->code = ERR_OUT_OF_MEMORY;
            return;
        }
        if(!push_step(context, root))
        {
            return;
        }

        if(has_more_input(context))
        {
            qualified_path(context);
        }
    }
    else
    {
        unexpected_value(context, '$');
    }
}

static void qualified_path(parser_context *context)
{
    enter_state(context, ST_QUALIFIED_PATH);
}

static void relative_path(parser_context *context)
{
    enter_state(context, ST_RELATIVE_PATH);

    context->code = SUCCESS;
    context->model->kind = RELATIVE_PATH;
    
}

static inline uint8_t get_char(parser_context *context)
{
    return context->input[context->cursor];
}

static inline void consume_char(parser_context *context)
{
    context->cursor++;
}

static inline bool has_more_input(parser_context *context)
{
    return context->length > context->cursor;
}

static step *make_root_step(void)
{
    return make_step(ROOT);
}

static inline step *make_step(enum kind kind)
{
    step *result = (step *)malloc(sizeof(step));
    if(NULL == result)
    {
        return NULL;
    }
    result->kind = kind;
    result->test.kind = NAME_TEST;
    result->test.name.prefix = NULL;
    result->test.name.prefix_length = 0;
    result->test.name.local = NULL;
    result->test.name.local_length = 0;
    result->predicate_count = 0;
    result->predicates = NULL;

    return result;
}

static bool push_step(parser_context *context, step *step)
{
    node *current = (node *)malloc(sizeof(node));
    if(NULL == current)
    {
        context->code = ERR_OUT_OF_MEMORY;
        return false;
    }
    current->step = step;
    current->next = NULL;
    context->model->length++;

    if(NULL == context->steps)
    {
        context->steps = current;
    }
    else
    {
        current->next = context->steps;
        context->steps = current;
    }
    return true;
}

static step *pop_step(parser_context *context)
{
    if(NULL == context->steps)
    {
        return NULL;
    }
    node *top = context->steps;
    step *result = top->step;
    context->steps = top->next;
    free(top);
    return result;
}

static inline bool validate(parser_result *result, uint8_t *expression, size_t length, jsonpath *model)
{
    if(NULL == result)
    {
        return false;
    }
    if(NULL == expression)
    {
        result->code = ERR_NULL_EXPRESSION;
        result->message = prepare_simple_message(result->code);
        return false;
    }
    if(0 == length)
    {
        result->code = ERR_ZERO_LENGTH;
        result->message = prepare_simple_message(result->code);
        return false;
    }
    if(NULL == model)
    {
        result->code = ERR_NULL_OUTPUT_PATH;
        result->message = prepare_simple_message(result->code);
        return false;
    }

    return true;
}

static inline void prepare_context(parser_context *context, uint8_t *expression, size_t length, jsonpath *path)
{
    path->length = 0;
    path->steps = NULL;
    
    context->input = expression;
    context->length = length;
    context->cursor = 0;
    context->state = ST_START;
    context->model = path;    
}

static inline void unexpected_value(parser_context *context, uint8_t expected)
{
    context->code = ERR_UNEXPECTED_VALUE;
    context->expected = expected;
}

static inline void enter_state(parser_context *context, enum state state)
{
    context->state = state;
    fprintf(stdout, "entering state: '%s'\n", STATES[state]);
}

static char *prepare_message(parser_context *context)
{
    char *message = NULL;
    
    switch(context->code)
    {
        case ERR_PREMATURE_END_OF_INPUT:
        case ERR_EXPECTED_INTEGER:
            asprintf(&message, MESSAGES[context->code], context->cursor);
            break;
        case ERR_UNEXPECTED_VALUE:
            asprintf(&message, MESSAGES[context->code], context->cursor, 
                     context->input[context->cursor], context->expected);
            break;
        default:
            message = prepare_simple_message(context->code);
            break;
    }

    return message;
}

static char *prepare_simple_message(jsonpath_status_code code)
{
    char *message = NULL;
    
    size_t len = strlen(MESSAGES[code]) + 1;
    message = (char *)malloc(len);
    if(NULL != message)
    {
        memcpy(message, (void *)MESSAGES[code], len);
    }

    return message;
}
