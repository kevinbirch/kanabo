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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct context
{
    uint8_t *input;
    size_t  length;
    size_t  cursor;
};

typedef struct context parser_context;

enum result
{
    SUCCESS = 0,
    ERR_EOF,
    ERR_UNEXPECTED_CHAR,
    ERR_EXPECTED_INTEGER
};

typedef enum result parser_result;

static const char *MESSAGES[] = 
{
    "Success",
    "Premature end of stream",
    "Unexpected char %c, was expecting %c instead",
    "Expected an integer, but found %c"
};
#pragma unused(MESSAGES)

struct error
{
    uint_fast16_t code;
    char *message;
};

typedef struct error error;

typedef parser_result (*parser)(parser_context *);
//typedef parser_result *(*combinator)(parser_result *(*first)(, ...);

// combinators
//parser_result *one(parser only);
//parser_result *concat(combinator one, ...);
//parser_result *choice(combinator one, ...);
//parser_result *optional(combinator one, ...);

// production parsers
parser_result path(parser_context *context);
parser_result absolute_path(parser_context *context);
parser_result relative_path(parser_context *context);

// terminal parsers
parser_result string(parser_context *context);
parser_result integer(parser_context *context);
parser_result string_literal(parser_context *context, char *value);

parser_result path(parser_context *context)
{
    // need a way to to push back from the input stream
    parser_result result = absolute_path(context);
    if(result == SUCCESS)
    {
        return result;
    }
    else
    {
        return relative_path(context);
    }
}

parser_result absolute_path(parser_context *context)
{
#pragma unused(context)
    return ERR_UNEXPECTED_CHAR;
}

parser_result relative_path(parser_context *context)
{
#pragma unused(context)
    return ERR_UNEXPECTED_CHAR;
}


