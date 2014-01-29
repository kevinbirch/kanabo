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

#include <stdbool.h>

#include "jsonpath/combinators.h"
#include "jsonpath/logging.h"
#include "vector.h"

struct sequence_context
{
    parser_context *context;
    MaybeAst        result;
};

typedef struct sequence_context sequence_context;

bool alternation_iterator(void *each, void *context);
bool concatenation_iterator(void *each, void *context);


MaybeAst rule_parser(parser_context *context, void *arg)
{
    // TODO is this necessary? couldn't the parser arg to the rule combinator be invoked instead?
    rule_context *combinator_context = (rule_context *)arg;
    parser_debug("entering rule: %s", combinator_context->name);

    MaybeAst result = combinator_context->expression->parser(context, combinator_context->expression->argument);

    parser_debug("leaving rule: %s, success: %s", combinator_context->name, VALUE == result.tag ? "yes" : "no");
    return result;
}

bool alternation_iterator(void *each, void *context)
{
    combinator *expression = (combinator *)each;
    log_combinator(expression);

    sequence_context *meta_context = (sequence_context *)context;

    // TODO reset error
    // TODO set input mark
    // TODO consume whitespace
    MaybeAst sub_result = expression->parser(meta_context->context, expression->argument);
    // TODO reset to mark
    // TODO log result
    if(VALUE == sub_result.tag)
    {
        meta_context->result = sub_result;
        return true;
    }

    return false;
}

MaybeAst alternation_parser(parser_context *context, void *arg)
{
    Vector *branches = (Vector *)arg;
    parser_debug("attempting %zd branches", vector_length(branches));

    sequence_context meta_context;
    meta_context.context = context;
    meta_context.result = collection();
    if(!vector_any(branches, alternation_iterator, &meta_context))
    {
        // need some way to capture the alternates for the error message
        return nothing(ERR_NO_ALTERNATIVE);
    }

    return meta_context.result;
}

bool concatenation_iterator(void *each, void *context)
{
    combinator *expression = (combinator *)each;
    log_combinator(expression);

    sequence_context *meta_context = (sequence_context *)context;

    // TODO consume whitespace
    MaybeAst sub_result = expression->parser(meta_context->context, expression->argument);
    if(VALUE == sub_result.tag)
    {
        ast_add_child(meta_context->result.value, sub_result.value);
        return true;
    }

    ast_free(meta_context->result.value);
    meta_context->result = sub_result;
    return false;
}

MaybeAst concatenation_parser(parser_context *context, void *arg)
{
    Vector *steps = (Vector *)arg;
    parser_debug("attempting %zd steps", vector_length(steps));

    sequence_context meta_context;
    meta_context.context = context;
    meta_context.result = collection();
    if(!vector_all(steps, concatenation_iterator, &meta_context))
    {
        // TODO bail out here
    }

    return meta_context.result;
}

MaybeAst option_parser(parser_context *context, void *arg)
{
    combinator *expression = (combinator *)arg;
    log_combinator(expression);

    MaybeAst sub_result = expression->parser(context, expression->argument);
    if(VALUE == sub_result.tag)
    {
        return sub_result;
    }

    return none();
}

MaybeAst repeat_parser(parser_context *context, void *arg)
{
    combinator *expression = (combinator *)arg;
    log_combinator(expression);

    MaybeAst result = collection();
    MaybeAst sub_result = expression->parser(context, expression->argument);
    while(VALUE == sub_result.tag)
    {
        ast_add_child(result.value, sub_result.value);
        sub_result = expression->parser(context, expression->argument);
    }

    return result;
}
