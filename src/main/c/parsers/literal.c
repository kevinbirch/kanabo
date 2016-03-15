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


#include "parsers/base.h"


struct literal_parser_s
{
    Parser   base;
    size_t   length;
    uint8_t  value[];
};

typedef struct literal_parser_s LiteralParser;


static MaybeSyntaxNode literal_delegate(Parser *parser, MaybeSyntaxNode node, Input *input)
{
    LiteralParser *self = (LiteralParser *)parser;

    ensure_more_input(input);
    skip_whitespace(input);
    if(consume_if(input, self->value, self->length))
    {
        // xxx - add literal to node
        return node;
    }
    else
    {
        return nothing_node(ERR_UNEXPECTED_VALUE);
    }
}

Parser *literal(const char *value)
{
    if(NULL == value)
    {
        return NULL;
    }
    size_t length = strlen(value);
    LiteralParser *self = calloc(1, sizeof(LiteralParser) + length);
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, LITERAL);
    self->base.vtable.delegate = literal_delegate;
    asprintf(&self->base.repr, "literal '%s'", value);
    memcpy(self->value, value, length);
    self->length = length;

    return (Parser *)self;
}
