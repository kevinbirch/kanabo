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
#include <stdarg.h>
#include <stdbool.h>

#include "jsonpath/combinators.h"
#include "jsonpath/logging.h"
#include "vector.h"

static const char * const COMBINATOR_NAMES[] =
{
    "alternation",
    "concatenation",
    "option",
    "repetition",
    "terminal",
    "rule"
};

static combinator *combinator_init(enum combinator_kind kind, parser_function parser, void *argument);
static combinator *make_sequence_combinator(enum combinator_kind kind, parser_function func, combinator *one, combinator *two, va_list rest);
static void combinator_destructor(void *each);


combinator *make_combinator(void)
{
    combinator *result = (combinator *)calloc(1, sizeof(combinator));
    if(NULL == result)
    {
        return NULL;
    }

    return result;
}


combinator *combinator_init(enum combinator_kind kind, parser_function parser, void *argument)
{
    combinator *result = make_combinator();
    if(NULL == result)
    {
        return NULL;
    }
    result->kind = kind;
    result->parser = parser;
    result->argument = argument;
    
    return result;
}

static void combinator_destructor(void *each)
{
    combinator_free((combinator *)each);
}

static combinator *make_sequence_combinator(enum combinator_kind kind, parser_function func, combinator *one, combinator *two, va_list rest)
{
    Vector *combinators = make_vector();
    if(NULL == combinators)
    {
        return NULL;
    }
    vector_add(combinators, one);
    vector_add(combinators, two);
    
    combinator *each = va_arg(rest, combinator *);
    while(NULL != each)
    {
        vector_add(combinators, each);
        each = va_arg(rest, combinator *);
    }

    combinator *result = combinator_init(kind, func, combinators);
    if(NULL == result)
    {
        vector_destroy(combinators, combinator_destructor);
        return NULL;
    }

    return result;
}

void combinator_free(combinator *value)
{
    if(NULL == value)
    {
        return;
    }
    switch(value->kind)
    {
        case ALTERNATION:
        case CONCATENATION:
            vector_destroy((Vector *)value->argument, combinator_destructor);
            break;
        case OPTION:
        case REPETITION:
            combinator_free((combinator *)value->argument);
            break;
        case RULE:
            combinator_free(((rule_context *)value->argument)->expression);
            break;
        case TERMINAL:
            break;
    }
    free(value);
}

combinator *rule_combinator(char *name, combinator *expression)
{
    if(NULL == expression)
    {
        return NULL;
    }
    rule_context *context = calloc(1, sizeof(rule_context));
    if(NULL == context)
    {
        return NULL;
    }

    context->name = name;
    context->expression = expression;
    
    combinator *result = combinator_init(RULE, rule_parser, context);
    if(NULL == result)
    {
        free(context);
        return NULL;
    }

    return result;
}

combinator *alternation_combinator(combinator *one, combinator *two, ...)
{
    if(NULL == one || NULL == two)
    {
        if(NULL != one)
        {
            combinator_free(one);
        }
        if(NULL != two)
        {
            combinator_free(two);
        }
        return NULL;
    }
    va_list rest;
    va_start(rest, two);
    combinator *result = make_sequence_combinator(ALTERNATION, alternation_parser, one, two, rest);
    va_end(rest);

    return result;
}

combinator *concatenation_combinator(combinator *one, combinator *two, ...)
{
    if(NULL == one || NULL == two)
    {
        if(NULL != one)
        {
            combinator_free(one);
        }
        if(NULL != two)
        {
            combinator_free(two);
        }
        return NULL;
    }
    va_list rest;
    va_start(rest, two);
    combinator *result = make_sequence_combinator(CONCATENATION, concatenation_parser, one, two, rest);
    va_end(rest);

    return result;
}

combinator *option(combinator *expression)
{
    if(NULL == expression)
    {
        return NULL;
    }
    return combinator_init(OPTION, option_parser, expression);
}

combinator *repetition(combinator *expression)
{
    if(NULL == expression)
    {
        return NULL;
    }
    return combinator_init(REPETITION, repeat_parser, expression);
}

combinator *literal(char *value)
{
    return combinator_init(TERMINAL, literal_parser, value);
}

combinator *number(void)
{
    return combinator_init(TERMINAL, number_parser, NULL);
}

combinator *integer(void)
{
    return combinator_init(TERMINAL, integer_parser, NULL);
}

combinator *signed_integer(void)
{
    return combinator_init(TERMINAL, signed_integer_parser, NULL);
}

combinator *non_zero_signed_integer(void)
{
    return combinator_init(TERMINAL, non_zero_signed_integer_parser, NULL);
}

combinator *quoted_string(void)
{
    return combinator_init(TERMINAL, quoted_string_parser, NULL);
}

combinator *string(void)
{
    return combinator_init(TERMINAL, string_parser, NULL);
}

#ifdef USE_LOGGING
void log_combinator(combinator *value)
{
    switch(value->kind)
    {
        case ALTERNATION:
        {
            Vector *branches = (Vector *)value->argument;
            parser_debug("processing alternation combinator, %zd branches", vector_length(branches));
        }
        break;
        case CONCATENATION:
        {
            Vector *steps = (Vector *)value->argument;
            parser_debug("processing concatenation combinator, %zd steps", vector_length(steps));
        }
        break;
        case OPTION:
        {
            combinator *contained = (combinator *)value->argument;
            parser_debug("processing option combinator, containing: %s", COMBINATOR_NAMES[contained->kind]);
        }
        break;
        case REPETITION:
        {
            combinator *contained = (combinator *)value->argument;
            parser_debug("processing repetition combinator, containing: %s", COMBINATOR_NAMES[contained->kind]);
        }
        break;
        case TERMINAL:
            parser_debug("processing termainal combinator, value: %s", (char *)value->argument);
            break;
        case RULE:
        {
            rule_context *context = (rule_context *)value->argument;
            parser_debug("processing rule combinator, name: %s", context->name);
        }
        break;
    }
}
#endif
