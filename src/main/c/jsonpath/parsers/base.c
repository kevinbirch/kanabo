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


#include "jsonpath/parsers/base.h"


static const char * const PARSER_NAMES[] =
{
    "rule",
    "choice",
    "sequence",
    "option",
    "repetition",
    "reference",
    "literal",
    "number",
    "integer",
    "signed integer",
    "non zero signed integer",
    "string"
};


static void base_free(Parser *self)
{
    free(self);
}

static MaybeAst base_delegate(Parser *self __attribute__((unused)),
                              MaybeAst ast,
                              Input *input __attribute__((unused)))
{
    return ast;
}

Parser *make_parser(enum parser_kind kind)
{
    Parser *parser = (Parser *)calloc(1, sizeof(Parser));
    return parser_init(parser, kind);
}

Parser *parser_init(Parser *self, enum parser_kind kind)
{
    if(NULL == self)
    {
        return NULL;
    }
    self->kind = kind;
    self->vtable.free = base_free;
    self->vtable.delegate = base_delegate;

    return self;
}

void parser_free(Parser *self)
{
    if(NULL == self)
    {
        return;
    }
    self->vtable.free(self);
    free(self);
}

void parser_destructor(void *each)
{
    parser_free((Parser *)each);
}

enum parser_kind parser_kind(Parser *self)
{
    return self->kind;
}

const char *parser_name(Parser *self)
{
    return PARSER_NAMES[parser_kind(self)];
}

const char *parser_repr(Parser *self)
{
    return NULL != self->repr ? (const char *)self->repr : parser_name(self);
}

bool is_terminal(Parser *self)
{
    return REPETITION < self->kind;
}

bool is_nonterminal(Parser *self)
{
    return !is_terminal(self);
}

MaybeAst bind(Parser *self, MaybeAst ast, Input *input)
{
    static size_t padding = 0;
    if(is_nothing(ast))
    {
        return ast;    
    }
    parser_trace("%*sentering %s", (2 * padding++), "", parser_repr(self));
    MaybeAst result = self->vtable.delegate(self, ast, input);
    parser_trace(
        "%*sleaving %s, %s", (2 * --padding), "",
        is_nonterminal(self) ? parser_repr(self) : parser_name(self),
        is_nothing(result) ? "failure" : "success");
    return result;
}
