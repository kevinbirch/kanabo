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


#include <stdio.h>

#include "parser/base.h"


struct literal_parser_s
{
    Parser   base;
    String   *value;
};

typedef struct literal_parser_s LiteralParser;


static MaybeSyntaxNode literal_delegate(Parser *parser, MaybeSyntaxNode node, Input *input)
{
    LiteralParser *self = (LiteralParser *)parser;

    ensure_more_input(input);
    skip_whitespace(input);
    const uint8_t *bytestring = (const uint8_t *)string_as_c_string(self->value);
    if(consume_if(input, bytestring, string_length(self->value)))
    {
        Location location = location_from_input(input);
        SyntaxNode *literal = make_syntax_node(CST_LITERAL, self->value, location);
        syntax_node_add_child(value(node), literal);

        return just_node(literal);
    }
    else
    {
        return nothing_node(ERR_PARSER_UNEXPECTED_VALUE);
    }
}

Parser *literal(const char *value)
{
    if(NULL == value)
    {
        return NULL;
    }
    LiteralParser *self = calloc(1, sizeof(LiteralParser));
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, LITERAL);
    self->base.vtable.delegate = literal_delegate;
    asprintf(&self->base.repr, "literal '%s'", value);
    self->value = make_string(value);

    return (Parser *)self;
}
