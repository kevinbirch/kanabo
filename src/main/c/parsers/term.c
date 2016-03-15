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


struct term_parser_s
{
    Parser           base;
    character_filter filter;
    String          *stop;
};

typedef struct term_parser_s TermParser;


MaybeString default_filter(Input *input)
{
    uint8_t current = consume_char(input);
    MutableString *result = make_mstring_with_char(current);
    if(NULL == result)
    {
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    return just_string(result);
}

static void string_parser_free(Parser *parser)
{
    TermParser *self = (TermParser *)parser;
    string_free(self->stop);
    free(self);
}

static inline bool is_stop_char(TermParser *self, uint8_t value)
{
    for(size_t i = 0; i < string_length(self->stop); i++)
    {
        if(value == string_get(self->stop, i))
        {
            return true;
        }
    }
    return false;
}

static MaybeSyntaxNode string_delegate(Parser *parser, MaybeSyntaxNode node, Input *input)
{
    skip_whitespace(input);
    ensure_more_input(input);

    TermParser *self = (TermParser *)parser;

    MutableString *result = make_mstring(4);
    while(true)
    {
        if(!has_more(input) || is_stop_char(self, peek(input)))
        {
            break;
        }
        MaybeString maybe = self->filter(input);
        if(is_nothing(maybe))
        {
            mstring_free(result);
            return nothing_node(code(maybe));
        }
        mstring_append(&result, value(maybe));
    }

    String *term = mstring_as_string(result);
    SyntaxNode *term_node = make_syntax_node(CST_TERM, term, location_from_input(input));
    syntax_node_add_child(value(node), term_node);
    mstring_free(result);
    return node;
}

Parser *term(character_filter filter, const char *stop_characters)
{
    if(NULL == filter)
    {
        return NULL;
    }
    TermParser *self = calloc(1, sizeof(TermParser));
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, STRING);
    self->base.vtable.delegate = string_delegate;
    self->base.vtable.free = string_parser_free;
    self->filter = filter;
    self->stop = make_string(stop_characters);

    return (Parser *)self;
}
