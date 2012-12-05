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
    "absolute path",
    "qualified path",
    "relative path",
    "abbreviated relative path",
    "step",
    "name test",
    "wildcard name test",
    "name",
    "node type test",
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
    ST_NAME,
    ST_NODE_TYPE_TEST,
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
    
    jsonpath *model;
    node *steps;

    enum state state;
    enum step_kind current_step_kind;

    jsonpath_status_code code;
    uint8_t expected;
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
    "Premature end of input after position %d",
    "At position %d: unexpected value '%c', was expecting '%c' instead",
    "At position %d: expected an integer"
};

// freeeeeedom!!!
static void free_step(step *step);
static void free_predicate(predicate *predicate);

// production parsers
static void path(parser_context *context);
static void absolute_path(parser_context *context);
static void qualified_path(parser_context *context);
static void relative_path(parser_context *context);
static void abbreviated_relative_path(parser_context *context);
static void path_step(parser_context *context);
static void name_test(parser_context *context);
static void wildcard_name(parser_context *context, step *name_test);
static void name(parser_context *context, step *name_test);
static void node_type_test(parser_context *context);

// input stream handling
static inline bool has_more_input(parser_context *context);
static inline size_t remaining(parser_context *context);
static bool look_for(parser_context *context, char *target);
static inline uint8_t get_char(parser_context *context);
static inline uint8_t peek(parser_context *context, size_t offset);
static inline void consume_char(parser_context *context);
static inline void consume_chars(parser_context *context, size_t count);
static inline void push_back(parser_context *context);

// step constructors
static step *make_root_step(void);
static inline step *make_step(enum step_kind step_kind, enum test_kind test_kind);

// state management
static bool push_step(parser_context *context, step *step);
static step *pop_step(parser_context *context);
static inline void enter_state(parser_context *context, enum state state);
static void unwind_context(parser_context *context);
static void abort_context(parser_context *context);
static inline void prepare_context(parser_context *context, uint8_t *expression, size_t length, jsonpath *path);
static inline bool validate(parser_result *result, uint8_t *expression, size_t length, jsonpath *model);

// error handlers
static inline void unexpected_value(parser_context *context, uint8_t expected);

// error message helpers
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
        if(NULL != step->test.name.value)
        {
            free(step->test.name.value);
            step->test.name.value = NULL;
            step->test.name.length = 0;
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

    fprintf(stdout, "starting expression: '");
    fwrite(expression, length, 1, stdout);
    fprintf(stdout, "'\n");
    
    parser_context context;
    prepare_context(&context, expression, length, model);
    
    path(&context);

    result->code = context.code;
    result->message = prepare_message(&context);
    result->position = context.cursor + 1;

    if(SUCCESS != context.code)
    {
        abort_context(&context);
        fprintf(stdout, "aborted. %s\n", result->message);
    }
    else
    {
        unwind_context(&context);
        fprintf(stdout, "done. found %zd steps.\n", model->length);
    }
    
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

    if(ERR_UNEXPECTED_VALUE == context->code && '$' == context->expected)
    {
        enter_state(context, ST_START);
        relative_path(context);
    }
}

static void absolute_path(parser_context *context)
{
    enter_state(context, ST_ABSOLUTE_PATH);

    if('$' == get_char(context))
    {
        context->code = SUCCESS;
        context->model->kind = ABSOLUTE_PATH;
        context->current_step_kind = ROOT;
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

    abbreviated_relative_path(context);
    if(SUCCESS == context->code || !has_more_input(context))
    {
        return;
    }
    else if('.' == get_char(context))
    {
        consume_char(context);
        context->current_step_kind = SINGLE;
        relative_path(context);
    }
    else
    {
        unexpected_value(context, '.');
    }
}

static void relative_path(parser_context *context)
{
    enter_state(context, ST_RELATIVE_PATH);

    if(!has_more_input(context))
    {
        context->code = ERR_PREMATURE_END_OF_INPUT;
        return;
    }

    if(look_for(context, "()"))
    {
        node_type_test(context);
    }
    else
    {
        path_step(context);
        if(SUCCESS == context->code && has_more_input(context))
        {
            qualified_path(context);
        }
    }
}

static void abbreviated_relative_path(parser_context *context)
{
    enter_state(context, ST_ABBREVIATED_RELATIVE_PATH);
    if('.' != get_char(context))
    {
        unexpected_value(context, '.');
        return;
    }
    consume_char(context);
    if('.' != get_char(context))
    {
        unexpected_value(context, '.');
        push_back(context);
        return;
    }
    consume_char(context);

    context->current_step_kind = RECURSIVE;
    relative_path(context);
}

static void path_step(parser_context *context)
{
    enter_state(context, ST_STEP);
    if(1 > remaining(context))
    {
        context->code = ERR_PREMATURE_END_OF_INPUT;
        return;
    }
    name_test(context);

    // xxx - if success, parse predicates here
}

static void name_test(parser_context *context)
{
    enter_state(context, ST_NAME_TEST);
    if(1 > remaining(context))
    {
        context->code = ERR_PREMATURE_END_OF_INPUT;
        return;
    }

    context->code = SUCCESS;
    step *current = make_step(context->current_step_kind, NAME_TEST);
    if(NULL == current)
    {
        context->code = ERR_OUT_OF_MEMORY;
        return;
    }
    push_step(context, current);

    if('*' == get_char(context))
    {
        wildcard_name(context, current);
    }
    else
    {
        name(context, current);
    }
}

static void wildcard_name(parser_context *context, step *name_step)
{
    enter_state(context, ST_WILDCARD_NAME_TEST);
    consume_char(context);
    name_step->test.name.value = (uint8_t *)malloc(1);
    if(NULL == name_step->test.name.value)
    {
        context->code = ERR_OUT_OF_MEMORY;
        return;
    }
    memcpy(name_step->test.name.value, "*", 1);
    name_step->test.name.length = 1;
}

static void name(parser_context *context, step *name_step)
{
    enter_state(context, ST_NAME);
    size_t offset = context->cursor;
    
    while(offset < context->length)
    {
        if('.' == context->input[offset] || '[' == context->input[offset])
        {
            break;
        }
        offset++;
    }

    bool quoted = false;
    if('\'' == get_char(context) && '\'' == context->input[offset - 1])
    {
        consume_char(context);
        offset--;
        quoted = true;
    }
    name_step->test.name.length = offset - context->cursor;
    name_step->test.name.value = (uint8_t *)malloc(name_step->test.name.length);
    if(NULL == name_step->test.name.value)
    {
        context->code = ERR_OUT_OF_MEMORY;
        return;
    }
    memcpy(name_step->test.name.value, context->input + context->cursor, name_step->test.name.length);
    consume_chars(context, name_step->test.name.length);
    if(quoted)
    {
        consume_char(context);
    }
}

static void node_type_test(parser_context *context)
{
    enter_state(context, ST_NODE_TYPE_TEST);

    // xxx - parse type tests here
    context->code = ERR_NOT_JSONPATH;
    /*
      scan ahead until '('
      if test is 0 length, error
      else create node and consume
      if test length is < smallest test, error
      else find matching test name
      if not found, error
      consume '('
      consume whitespace
      if current is not ')', error
      else consume ')'
     */
}

static bool look_for(parser_context *context, char *target)
{
    size_t offset = context->cursor;
    size_t index = 0;
    size_t length = strlen(target);
    while(offset < context->length)
    {
        if('.' == context->input[offset])
        {
            return false;
        }
        if(target[index] == context->input[offset])
        {
            index++;
            if(index == length)
            {
                return true;
            }
        }
        offset++;
    }
    return false;
}

static inline uint8_t get_char(parser_context *context)
{
    return context->input[context->cursor];
}

static inline uint8_t peek(parser_context *context, size_t offset)
{
    return context->input[context->cursor + offset];
}

static inline void consume_char(parser_context *context)
{
    context->cursor++;
}

static inline void consume_chars(parser_context *context, size_t count)
{
    context->cursor += count;
}

static inline void push_back(parser_context *context)
{
    context->cursor--;
}

static inline size_t remaining(parser_context *context)
{
    return (context->length - 1) - context->cursor;
}

static inline bool has_more_input(parser_context *context)
{
    return context->length > context->cursor;
}

static step *make_root_step(void)
{
    return make_step(ROOT, NAME_TEST);
}

static inline step *make_step(enum step_kind step_kind, enum test_kind test_kind)
{
    step *result = (step *)malloc(sizeof(step));
    if(NULL == result)
    {
        return NULL;
    }
    result->kind = step_kind;
    result->test.kind = test_kind;
    result->test.name.value = NULL;
    result->test.name.length = 0;
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
    context->model->kind = RELATIVE_PATH;
}

static inline void unexpected_value(parser_context *context, uint8_t expected)
{
    context->code = ERR_UNEXPECTED_VALUE;
    context->expected = expected;
}

static inline void enter_state(parser_context *context, enum state state)
{
    context->state = state;
    //fprintf(stdout, "entering state: '%s'\n", STATES[state]);
}

static char *prepare_message(parser_context *context)
{
    char *message = NULL;
    
    switch(context->code)
    {
        case ERR_PREMATURE_END_OF_INPUT:
            asprintf(&message, MESSAGES[context->code], context->cursor);
            break;
        case ERR_EXPECTED_INTEGER:
            asprintf(&message, MESSAGES[context->code], context->cursor + 1);
            break;
        case ERR_UNEXPECTED_VALUE:
            asprintf(&message, MESSAGES[context->code], context->cursor + 1,
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