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

struct context
{
    uint8_t *input;
    size_t  length;
    size_t  cursor;

    uint8_t expected;
    
    jsonpath *model;
    
    enum
    {
        START,
        ABSOLUTE_PATH,
        QUALIFIED_PATH,
        RELATIVE_PATH,
        ABBREVIATED_RELATIVE_PATH,
        STEP,
        NAME_TEST,
        WILDCARD_NAME_TEST,
        QNAME_TEST,
        TYPE_TEST,
        JSON_OBJECT_TYPE_TEST,
        JSON_ARRAY_TYPE_TEST,
        JSON_STRING_TYPE_TEST,
        JSON_NUMBER_TYPE_TEST,
        JSON_BOOLEAN_TYPE_TEST,
        JSON_NULL_TYPE_TEST,
        PREDICATE,
        WILDCARD_PREDICATE,
        SUBSCRIPT_PREDICATE,
        SLICE_PREDICATE,
        JOIN_PREDICATE,
        FILTER_PREDICATE,
        SCRIPT_PREDICATE
    } state;
    
    status_code code;
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

static inline void prepare_context(parser_context *context, uint8_t *expression, size_t length, jsonpath *path);
static inline bool validate(parser_result *result, uint8_t *expression, size_t length, jsonpath *model);
static inline char *prepare_message(parser_context *context);
static inline char *prepare_simple_message(status_code code);

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
static void relative_path(parser_context *context);

// terminal parsers
/* static void string(parser_context *context); */
/* static void integer(parser_context *context); */
/* static void string_literal(parser_context *context, char *value); */

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
    result->position = 0;
    if(!validate(result, expression, length, model))
    {
        return result;
    }

    parser_context context;
    prepare_context(&context, expression, length, model);
    
    path(&context);

    result->code = context.code;
    result->message = prepare_message(&context);
    result->position = context.cursor;
    
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
    context->state = START;
    context->model = path;    
}

static inline char *prepare_message(parser_context *context)
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

static inline char *prepare_simple_message(status_code code)
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

static void path(parser_context *context)
{
    context->state = START;
    absolute_path(context);

    context->state = START;
    relative_path(context);

    context->code = ERR_NOT_JSONPATH;
}

static void absolute_path(parser_context *context)
{
    context->state = ABSOLUTE_PATH;

}

static void relative_path(parser_context *context)
{
    context->state = RELATIVE_PATH;
    
}


