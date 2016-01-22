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


struct literal_parser_s
{
    Parser      base;
    const char *value;
};

typedef struct literal_parser_s LiteralParser;


static MaybeAst literal_delegate(Parser *parser, MaybeAst ast, Input *input)
{
    LiteralParser *self = (LiteralParser *)parser;

    ensure_more_input(input);
    skip_whitespace(input);
    parser_trace("checking for literal '%s'", self->value);
    if(consume_if(input, self->value))
    {
        // xxx - add literal to ast
        return ast;
    }
    else
    {
        // xxx - need to return the expected literal here! `error_with_context`? error structs and handlers?
        return error(ERR_UNEXPECTED_VALUE);
    }
}

Parser *literal(const char *value)
{
    if(NULL == value)
    {
        return NULL;
    }
    LiteralParser *self = (LiteralParser *)calloc(1, sizeof(LiteralParser));
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, LITERAL);
    self->base.vtable.delegate = literal_delegate;
    self->value = value;

    return (Parser *)self;
}
