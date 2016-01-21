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


#include "vector.h"

#include "jsonpath/parsers/compound.h"


struct sequence_context_s
{
    Input    *input;
    MaybeAst  ast;
};

typedef struct sequence_context_s Context;


static bool choice_iterator(void *each, void *paramter)
{
    Parser *expression = (Parser *)each;
    Context *context = (Context *)paramter;

    set_mark(context->input);
    skip_whitespace(context->input);    

    parser_trace("attempting choice parser branch: %s", parser_name(expression));
    MaybeAst result = bind(expression, context->ast, context->input);
    if(AST_VALUE == result.tag)
    {
        ast_add_child(context->ast.value, result.value);
        return true;
    }

    reset_to_mark(context->input);
    return false;
}

static MaybeAst choice_delegate(Parser *parser, MaybeAst ast, Input *input)
{
    CompoundParser *self = (CompoundParser *)parser;
    parser_trace("entering choice parser, %zd branches", vector_length(self->children));

    Context context = {input, ast};
    if(!vector_any(self->children, choice_iterator, &context))
    {
        // TODO free ast
        parser_trace("leaving choice parser, failure");
        return error(ERR_NO_ALTERNATIVE);
    }

    parser_trace("leaving choice parser, success");
    return ast;
}

Parser *choice_parser(Parser *one, Parser *two, ...)
{
    if(NULL == one || NULL == two)
    {
        if(NULL != one)
        {
            parser_free(one);
        }
        if(NULL != two)
        {
            parser_free(two);
        }
        return NULL;
    }
    parser_trace("building choice parser");
    va_list rest;
    va_start(rest, two);
    CompoundParser *self = make_compound_parser(CHOICE, one, two, rest);
    va_end(rest);
    if(NULL == self)
    {
        return NULL;
    }
    self->base.vtable.delegate = choice_delegate;

    return (Parser *)self;
}
