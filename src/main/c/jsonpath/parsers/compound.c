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


struct compound_parser_s
{
    Parser  base;
    Vector *children;
};

typedef struct compound_parser_s CompoundParser;


static void compound_free(Parser *value)
{
    CompoundParser *self = (CompoundParser *)value;
    vector_destroy(self->children, parser_destructor);
}

static void compound_log(Parser *value)
{
    CompoundParser *self = (CompoundParser *)value;
    parser_debug("processing %s parser, %zd steps",
                 parser_name(value), vector_length(self->children));
}

static const struct vtable_s COMPOUND_VTABLE =
{
    compound_free,
    compound_log
};

Parser *make_compound_parser(enum parser_kind kind, parser_function func,
                             Parser *one, Parser *two, va_list rest)
{
    CompoundParser *result = (CompoundParser *)calloc(1, sizeof(CompoundParser));
    if(NULL == result)
    {
        return NULL;
    }
    
    Vector *children = make_vector();
    if(NULL == children)
    {
        parser_free((Parser *)result);
        return NULL;
    }
    vector_add(children, one);
    vector_add(children, two);

    Parser *each = va_arg(rest, Parser *);
    while(NULL != each)
    {
        vector_add(children, each);
        each = va_arg(rest, Parser *);
    }

    parser_init((Parser *)result, kind, func, &COMPOUND_VTABLE);
    result->children = children;

    return (Parser *)result;
}

