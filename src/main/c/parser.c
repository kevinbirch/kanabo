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
#include <ctype.h>
#include <inttypes.h>
#include <errno.h>

#include "jsonpath.h"
#include "conditions.h"
#include "log.h"

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

// production parsers
static void path(parser_context *context);
static void absolute_path(parser_context *context);
static void qualified_path(parser_context *context);
static void relative_path(parser_context *context);
static void step_parser(parser_context *context);
static void abbreviated_relative_path(parser_context *context);
static void name_test(parser_context *context);
static void wildcard_name(parser_context *context);
static void step_predicate_parser(parser_context *context);
static void wildcard_predicate(parser_context *context);
static void subscript_predicate(parser_context *context);
static void slice_predicate(parser_context *context);
static void name(parser_context *context, step *name_test);
static void node_type_test(parser_context *context);

// parser helpers
static uint_fast32_t integer(parser_context *context);
static int_fast32_t signed_integer(parser_context *context);
static int32_t node_type_test_value(parser_context *context, size_t length);
static inline int32_t check_one_node_type_test_value(parser_context *context, size_t length, const char *target, enum type_test_kind result);

// input stream handling
static inline bool has_more_input(parser_context *context);
static inline size_t remaining(parser_context *context);
static bool look_for(parser_context *context, char *target);
static int_fast32_t offset_of(parser_context *context, char *target);
static inline uint8_t get_char(parser_context *context);
static inline uint8_t peek(parser_context *context, size_t offset);
static inline void skip_ws(parser_context *context);
static inline void consume_char(parser_context *context);
static inline void consume_chars(parser_context *context, size_t count);
static inline void push_back(parser_context *context);
static inline void reset(parser_context *context, size_t mark);

// step constructors
static step *make_root_step(void);
static inline step *make_step(enum step_kind step_kind, enum test_kind test_kind);

// state management
static void unwind_context(parser_context *context);
static bool push_step(parser_context *context, step *step);
static step *pop_step(parser_context *context);
static inline void enter_state(parser_context *context, enum state state);
static predicate *add_predicate(parser_context *context, enum predicate_kind kind);

// error handlers
static inline void unexpected_value(parser_context *context, uint8_t expected);

extern void step_free(step *step);

#define component_name "parser"

#define parser_info(FORMAT, ...)  log_info(component_name, FORMAT, ##__VA_ARGS__)
#define parser_debug(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define parser_trace(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, ...) log_string(TRACE, component_name, FORMAT, ##__VA_ARGS__)
#define debug_string(FORMAT, ...) log_string(DEBUG, component_name, FORMAT, ##__VA_ARGS__)

parser_context *make_parser(const uint8_t *expression, size_t length)
{
    parser_context *context = (parser_context *)calloc(1, sizeof(parser_context));
    if(NULL == context)
    {
        return NULL;
    }
    jsonpath *path = (jsonpath *)calloc(1, sizeof(jsonpath));
    if(NULL == context)
    {
        free(context);
        return NULL;
    }
    if(NULL == expression)
    {
        context->result.code = ERR_NULL_EXPRESSION;
        errno = EINVAL;
        return context;
    }
    if(0 == length)
    {
        context->result.code = ERR_ZERO_LENGTH;
        errno = EINVAL;
        return context;
    }
    
    context->path = path;    
    context->input = expression;
    context->length = length;
    context->cursor = 0;
    context->state = ST_START;
    context->path->kind = RELATIVE_PATH;
    context->result.code = JSONPATH_SUCCESS;
    context->result.expected_char = 0;
    context->result.actual_char = 0;

    return context;
}

enum parser_status_code parser_status(parser_context *context)
{
    return context->result.code;
}

jsonpath *parse(parser_context *context)
{
    PRECOND_NONNULL_ELSE_NULL(context);
    PRECOND_NONNULL_ELSE_NULL(context->path);
    PRECOND_NONNULL_ELSE_NULL(context->input);
    PRECOND_ELSE_NULL(0 != context->length);

    debug_string("starting expression: '%s'", context->input, context->length);
    path(context);

    if(JSONPATH_SUCCESS == context->result.code)
    {
        unwind_context(context);
        if(JSONPATH_SUCCESS == context->result.code)
        {
            parser_debug("done. found %zd steps.", context->path->length);
        }
        else
        {
#ifdef USE_LOGGING
            char *message = parser_status_message(context);
            parser_debug("aborted. unable to create jsonpath model. status: %d (%s)", context->result.code, message);
            free(message);
#endif
            return NULL;
        }
    }
    else
    {
        context->result.actual_char = context->input[context->cursor];        
#ifdef USE_LOGGING
        char *message = parser_status_message(context);
        parser_debug("aborted. %d (%s)", context->result.code, message);
        free(message);
#endif
        return NULL;
    }

    return context->path;
}

static void unwind_context(parser_context *context)
{
    context->path->steps = (step **)calloc(1, sizeof(step *) * context->path->length);
    if(NULL == context->path->steps)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return;
    }

    for(size_t i = 0; i < context->path->length; i++)
    {
        context->path->steps[(context->path->length - 1) - i] = pop_step(context);
    }
}

static void path(parser_context *context)
{
    enter_state(context, ST_START);
    skip_ws(context);
    
    absolute_path(context);

    if(ERR_UNEXPECTED_VALUE == context->result.code && '$' == context->result.expected_char)
    {
        enter_state(context, ST_START);
        context->current_step_kind = SINGLE;
        relative_path(context);
    }
}

static void absolute_path(parser_context *context)
{
    enter_state(context, ST_ABSOLUTE_PATH);

    if('$' == get_char(context))
    {
        context->result.code = JSONPATH_SUCCESS;
        context->path->kind = ABSOLUTE_PATH;
        context->current_step_kind = ROOT;
        consume_char(context);

        step *root = make_root_step();
        if(NULL == root)
        {
            context->result.code = ERR_PARSER_OUT_OF_MEMORY;
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
    skip_ws(context);

    abbreviated_relative_path(context);
    skip_ws(context);
    if(JSONPATH_SUCCESS == context->result.code || !has_more_input(context))
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

static void abbreviated_relative_path(parser_context *context)
{
    enter_state(context, ST_ABBREVIATED_RELATIVE_PATH);

    size_t mark = context->cursor;
    skip_ws(context);
    if('.' != get_char(context))
    {
        unexpected_value(context, '.');
        return;
    }
    consume_char(context);
    skip_ws(context);
    if('.' != get_char(context))
    {
        unexpected_value(context, '.');
        reset(context, mark);
        return;
    }
    consume_char(context);

    context->current_step_kind = RECURSIVE;
    relative_path(context);
}

static void relative_path(parser_context *context)
{
    enter_state(context, ST_RELATIVE_PATH);
    skip_ws(context);

    if(!has_more_input(context))
    {
        context->result.code = ERR_PREMATURE_END_OF_INPUT;
        return;
    }
    if('.' == get_char(context))
    {
        context->result.code = ERR_EXPECTED_NAME_CHAR;
        return;
    }

    step_parser(context);
    if(JSONPATH_SUCCESS == context->result.code && has_more_input(context))
    {
        qualified_path(context);
    }

}

static void step_parser(parser_context *context)
{
    if(look_for(context, "()"))
    {
        node_type_test(context);
        if(JSONPATH_SUCCESS == context->result.code && has_more_input(context))
        {
            consume_char(context);
            consume_char(context);
        }
    }
    else
    {
        name_test(context);
    }

    if(JSONPATH_SUCCESS == context->result.code && has_more_input(context))
    {
        step_predicate_parser(context);
    }

    if(ERR_UNEXPECTED_VALUE == context->result.code && '[' == context->result.expected_char)
    {
        enter_state(context, ST_STEP);
        context->result.code = JSONPATH_SUCCESS;
    }
}

static void wildcard_name(parser_context *context)
{
    enter_state(context, ST_WILDCARD_NAME_TEST);

    step *current = make_step(context->current_step_kind, WILDCARD_TEST);
    if(NULL == current)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return;
    }
    push_step(context, current);

    consume_char(context);
    context->result.code = JSONPATH_SUCCESS;
}

static void node_type_test(parser_context *context)
{
    enter_state(context, ST_NODE_TYPE_TEST);

    step *current = make_step(context->current_step_kind, TYPE_TEST);
    if(NULL == current)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return;
    }
    push_step(context, current);

    size_t offset = context->cursor;
    while(offset < context->length)
    {
        if('(' == context->input[offset])
        {
            break;
        }
        offset++;
    }
    size_t length = offset - context->cursor;
    if(0 == length)
    {
        context->result.code = ERR_EXPECTED_NODE_TYPE_TEST;
        return;
    }

    int32_t kind = node_type_test_value(context, length);
    if(-1 == kind)
    {
        context->result.code = ERR_EXPECTED_NODE_TYPE_TEST;
        return;
    }
    current->test.type = (enum type_test_kind)kind;
    
    consume_chars(context, length);
    context->result.code = JSONPATH_SUCCESS;
}

static int32_t node_type_test_value(parser_context *context, size_t length)
{
    int32_t result;
    
    switch(get_char(context))
    {
        case 'o':
            result = check_one_node_type_test_value(context, length, "object", OBJECT_TEST);
            break;
        case 'a':
            result = check_one_node_type_test_value(context, length, "array", ARRAY_TEST);
            break;
        case 's':
            result = check_one_node_type_test_value(context, length, "string", STRING_TEST);
            break;
        case 'n':
            result = check_one_node_type_test_value(context, length, "number", NUMBER_TEST);
            if(-1 == result)
            {
                result = check_one_node_type_test_value(context, length, "null", NULL_TEST);
            }
            break;
        case 'b':
            result = check_one_node_type_test_value(context, length, "boolean", BOOLEAN_TEST);
            break;
        default:
            result = -1;
            break;
    }

    return result;
}

static inline int32_t check_one_node_type_test_value(parser_context *context, size_t length, const char *target, enum type_test_kind result)
{
    if(strlen(target) == length  && 0 == memcmp(target, context->input + context->cursor, length))
    {
        return result;
    }
    else
    {
        return -1;
    }
}

static void name_test(parser_context *context)
{
    enter_state(context, ST_NAME_TEST);
    if('*' == get_char(context))
    {
        wildcard_name(context);
        return;
    }

    step *current = make_step(context->current_step_kind, NAME_TEST);
    if(NULL == current)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return;
    }
    push_step(context, current);

    context->result.code = JSONPATH_SUCCESS;
    name(context, current);
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
    while(isspace(context->input[offset - 1]))
    {
        offset--;
    }
    bool quoted = false;
    if('\'' == get_char(context) && '\'' == context->input[offset - 1])
    {
        consume_char(context);
        offset--;
        quoted = true;
    }
    name_step->test.name.length = offset - context->cursor;
    if(0 == name_step->test.name.length)
    {
        context->result.code = ERR_EXPECTED_NAME_CHAR;
        return;
    }
    name_step->test.name.value = (uint8_t *)calloc(1, name_step->test.name.length);
    if(NULL == name_step->test.name.value)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return;
    }
    memcpy(name_step->test.name.value, context->input + context->cursor, name_step->test.name.length);
    consume_chars(context, name_step->test.name.length);
    if(quoted)
    {
        consume_char(context);
    }
    skip_ws(context);
}

#define try_predicate_parser(PARSER) PARSER(context);              \
    if(JSONPATH_SUCCESS == context->result.code)                          \
    {                                                              \
        skip_ws(context);                                          \
        if(']' == get_char(context))                               \
        {                                                          \
            consume_char(context);                                 \
        }                                                          \
        else                                                       \
        {                                                          \
            context->result.code = ERR_EXTRA_JUNK_AFTER_PREDICATE;        \
        }                                                          \
        return;                                                    \
    }

static void step_predicate_parser(parser_context *context)
{
    enter_state(context, ST_PREDICATE);

    skip_ws(context);
    if('[' == get_char(context))
    {
        consume_char(context);
        if(!look_for(context, "]"))
        {
            context->result.code = ERR_UNBALANCED_PRED_DELIM;
            return;
        }
        skip_ws(context);
        if(']' == get_char(context))
        {
            context->result.code = ERR_EMPTY_PREDICATE;
            return;
        }

        try_predicate_parser(wildcard_predicate);
        try_predicate_parser(subscript_predicate);
        try_predicate_parser(slice_predicate);

        if(JSONPATH_SUCCESS != context->result.code && ERR_PARSER_OUT_OF_MEMORY != context->result.code)
        {
            context->result.code = ERR_UNSUPPORTED_PRED_TYPE;
        }
    }
    else
    {
        unexpected_value(context, '[');
    }
}

static void wildcard_predicate(parser_context *context)
{
    enter_state(context, ST_WILDCARD_PREDICATE);

    skip_ws(context);
    if('*' == get_char(context))
    {
        context->result.code = JSONPATH_SUCCESS;
        consume_char(context);
        add_predicate(context, WILDCARD);
    }
    else
    {
        unexpected_value(context, '*');
    }
}

static void subscript_predicate(parser_context *context)
{
    enter_state(context, ST_SUBSCRIPT_PREDICATE);

    size_t mark = context->cursor;
    uint_fast32_t subscript = integer(context);
    if(JSONPATH_SUCCESS != context->result.code)
    {
        reset(context, mark);
        return;
    }
    skip_ws(context);
    if(']' != get_char(context))
    {
        reset(context, mark);
        context->result.code = ERR_EXTRA_JUNK_AFTER_PREDICATE;
        return;
    }
    predicate *pred = add_predicate(context, SUBSCRIPT);
    pred->subscript.index = (size_t)subscript;
}

static uint_fast32_t integer(parser_context *context)
{
    skip_ws(context);
    if('-' == get_char(context))
    {
        context->result.code = ERR_EXPECTED_INTEGER;
        return 0;
    }
    char *begin = (char *)context->input + context->cursor;
    char *end;
    errno = 0;
    uint_fast32_t value = (uint_fast32_t)strtoul(begin, &end, 10);
    if(0 != errno || begin == end)
    {
        context->result.code = ERR_INVALID_NUMBER;
        return 0;
    }
    context->result.code = JSONPATH_SUCCESS;
    consume_chars(context, (size_t)(end - begin));
    return value;
}

static void slice_predicate(parser_context *context)
{
    enter_state(context, ST_SLICE_PREDICATE);

    int_fast32_t from = INT_FAST32_MIN;
    int_fast32_t to = INT_FAST32_MAX;
    int_fast32_t step = 1;
    predicate *pred = add_predicate(context, SLICE);

    if(!look_for(context, ":"))
    {
        parser_trace("slice: uh oh! no ':' found, aborting...");
        context->result.code = ERR_UNSUPPORTED_PRED_TYPE;
        return;
    }

    skip_ws(context);
    if(isdigit(get_char(context)) || '-' == get_char(context) || '+' == get_char(context))
    {
        parser_trace("slice: parsing from value...");
        from = signed_integer(context);
        if(JSONPATH_SUCCESS != context->result.code)
        {
            parser_trace("slice: uh oh! couldn't parse from value, aborting...");
            return;
        }
        parser_trace("slice: found from value: %d", to);
        pred->slice.specified |= SLICE_FROM;
    }
    else
    {
        parser_trace("slice: no from value specified");
    }
    skip_ws(context);
    if(':' != get_char(context))
    {
        parser_trace("slice: uh oh! missing ':' between from and to, aborting...");
        unexpected_value(context, ':');
        return;
    }
    consume_char(context);
    skip_ws(context);
    if(isdigit(get_char(context)) || '-' == get_char(context) || '+' == get_char(context))
    {
        parser_trace("slice: parsing to value...");
        to = signed_integer(context);
        if(JSONPATH_SUCCESS != context->result.code)
        {
            parser_trace("slice: uh oh! couldn't parse to value, aborting...");
            return;
        }
        parser_trace("slice: found to value: %d", to);
        pred->slice.specified |= SLICE_TO;
    }
    else
    {
        parser_trace("slice: no to value specified");    
    }
    skip_ws(context);
    if(':' == get_char(context))
    {
        consume_char(context);
        skip_ws(context);
        if(isdigit(get_char(context)) || '-' == get_char(context) || '+' == get_char(context))
        {
            parser_trace("slice: parsing step value...");
            step = signed_integer(context);
            if(JSONPATH_SUCCESS != context->result.code)
            {
                parser_trace("slice: uh oh! couldn't parse step value, aborting...");
                return;
            }
            if(0 == step)
            {
                parser_trace("slice: uh oh! couldn't parse step value, aborting...");
                context->result.code = ERR_STEP_CANNOT_BE_ZERO;
                return;
            }
            parser_trace("slice: found step value: %d", step);
            pred->slice.specified |= SLICE_STEP;
        }
        else
        {
            parser_trace("slice: no step value specified");
        }

    }

    context->result.code = JSONPATH_SUCCESS;
    pred->slice.from = from;
    pred->slice.to = to;
    pred->slice.step = step;
}

static int_fast32_t signed_integer(parser_context *context)
{
    skip_ws(context);
    char *begin = (char *)context->input + context->cursor;
    char *end;
    errno = 0;
    int_fast32_t value = (int_fast32_t)strtol(begin, &end, 10);
    if(0 != errno || begin == end)
    {
        context->result.code = ERR_INVALID_NUMBER;
        return 0;
    }
    context->result.code = JSONPATH_SUCCESS;
    consume_chars(context, (size_t)(end - begin));
    return value;
}

static predicate *add_predicate(parser_context *context, enum predicate_kind kind)
{
    predicate *pred = (predicate *)calloc(1, sizeof(struct predicate));
    if(NULL == pred)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return NULL;
    }

    pred->kind = kind;
    step *current = context->steps->step;
    current->predicate = pred;

    return pred;
}

static bool look_for(parser_context *context, char *target)
{
    return -1 != offset_of(context, target);
}

static int_fast32_t offset_of(parser_context *context, char *target)
{
    size_t offset = context->cursor;
    size_t index = 0;
    size_t length = strlen(target);

    while(offset < context->length)
    {
        if('.' == context->input[offset])
        {
            return -1;
        }
        if(target[index] == context->input[offset])
        {
            index++;
            if(index == length)
            {
                return (int_fast32_t)offset;
            }
        }
        offset++;
    }
    
    return offset == context->length ? -1 : (int_fast32_t)offset;
}

static inline uint8_t get_char(parser_context *context)
{
    return context->input[context->cursor];
}

static inline uint8_t peek(parser_context *context, size_t offset)
{
    return context->input[context->cursor + offset];
}

static inline void skip_ws(parser_context *context)
{
    while(isspace(get_char(context)))
    {
        consume_char(context);
    }
}
static inline void consume_char(parser_context *context)
{
    context->cursor++;
}

static inline void consume_chars(parser_context *context, size_t count)
{
    if(context->cursor + count > context->length)
    {
        context->cursor = context->length - 1;
    }
    else
    {
        context->cursor += count;
    }
}

static inline void push_back(parser_context *context)
{
    context->cursor--;
}

static inline void reset(parser_context *context, size_t mark)
{
    if(mark < context->cursor)
    {
        context->cursor = mark;
    }
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
    step *result = (step *)calloc(1, sizeof(step));
    if(NULL == result)
    {
        return NULL;
    }
    result->kind = step_kind;
    result->test.kind = test_kind;
    result->test.name.value = NULL;
    result->test.name.length = 0;
    result->predicate = NULL;

    return result;
}

static bool push_step(parser_context *context, step *value)
{
    cell *current = (cell *)calloc(1, sizeof(cell));
    if(NULL == current)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return false;
    }
    current->step = value;
    current->next = NULL;
    context->path->length++;

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
    cell *top = context->steps;
    step *result = top->step;
    context->steps = top->next;
    free(top);
    return result;
}

static inline void unexpected_value(parser_context *context, uint8_t expected)
{
    context->result.code = ERR_UNEXPECTED_VALUE;
    context->result.expected_char = expected;
}

static inline void enter_state(parser_context *context, enum state state)
{
    context->state = state;
    parser_trace("entering state: '%s'", STATES[state]);
}

