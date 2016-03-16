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

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#ifdef __APPLE__
#define _DARWIN_SOURCE
#endif

#include <stdio.h>
#include <string.h>

#include "parsers.h"
#include "conditions.h"

static const char * const MESSAGES[] =
{
    "Success.",
    "Expression was NULL.",
    "Expression length was zero.",
    "Unable to allocate memory.",
    "At position %zd: premature end of input.",
    "At position %zd: unexpected character.",
    "At position %zd: expected a name character.",
    "At position %zd: invalid control character.",
    "At position %zd: unsupported escape sequence.",
    "At position %zd: empty predicate.",
    "At position %zd: missing closing predicate delimiter `]' before end of step.",
    "At position %zd: unsupported predicate found.",
    "At position %zd: extra characters after valid predicate definition.",
    "At position %zd: expected a node type test.",
    "At position %zd: expected an integer.",
    "At position %zd: invalid number.",
    "At position %zd: slice step value must be non-zero."
};

char *parser_status_message(parser_result_code code, size_t reported_position)
{
    char *message = NULL;
    int result = 0;
    size_t position = reported_position + 1;

    switch(code)
    {
        case ERR_PREMATURE_END_OF_INPUT:
            position--;
        case ERR_EXPECTED_NODE_TYPE_TEST:
        case ERR_EMPTY_PREDICATE:
        case ERR_UNBALANCED_PRED_DELIM:
        case ERR_EXTRA_JUNK_AFTER_PREDICATE:
        case ERR_UNSUPPORTED_PRED_TYPE:
        case ERR_EXPECTED_INTEGER:
        case ERR_INVALID_NUMBER:
        case ERR_UNEXPECTED_VALUE:
        case ERR_EXPECTED_NAME_CHAR:
            result = asprintf(&message, MESSAGES[code], position);
            break;
        default:
            message = strdup(MESSAGES[code]);
            break;
    }
    if(-1 == result)
    {
        message = NULL;
    }

    return message;
}
