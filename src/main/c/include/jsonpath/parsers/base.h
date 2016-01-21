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


#pragma once


#include "jsonpath/maybe_ast.h"
#include "jsonpath/logging.h"


enum parser_kind
{
    RULE,
    CHOICE,
    SEQUENCE,
    OPTION,
    REPETITION,
    REFERENCE,
    LITERAL,
    NUMBER,
    INTEGER,
    SIGNED_INTEGER,
    NON_ZERO_SIGNED_INTEGER,
    QUOTED_STRING,
    STRING
};

struct parser_s
{
    enum parser_kind kind;
    struct
    {
        void (*free)(Parser *self);
        MaybeAst (*delegate)(Parser *parser, MaybeAst ast, Input *input);
    } vtable;
};


Parser *make_parser(enum parser_kind kind);
Parser *parser_init(Parser *self, enum parser_kind kind);

void parser_free(Parser *self);
void parser_destructor(void *each);

enum parser_kind parser_kind(Parser *self);
const char *parser_name(Parser *self);
