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
#include <string.h>

#include "jsonpath.h"

static const char * const MESSAGES[] = 
{
    "Success",
    "Expression is NULL",
    "Expression length is 0",
    "Output path is NULL",
    "Unable to allocate memory",
    "Not a JSONPath expression",
    "Premature end of input after position %d",
    "At position %d: unexpected character '%c', was expecting '%c' instead",
    "At position %d: empty predicate",
    "At position %d: missing closing predicate delimiter `]' before end of step",
    "At position %d: unsupported predicate",
    "At position %d: extra characters after valid predicate definition",
    "At position %d: expected a name character, but found '%c' instead",
    "At position %d: expected a node type test",
    "At position %d: expected an integer"
    "At position %d: invalid integer"
};

static char *make_simple_status_message(jsonpath_status_code code);

char *make_status_message(const jsonpath * restrict path)
{
    char *message = NULL;
    
    switch(path->result.code)
    {
        case ERR_PREMATURE_END_OF_INPUT:
            asprintf(&message, MESSAGES[path->result.code], path->result.position);
            break;
        case ERR_EXPECTED_NODE_TYPE_TEST:
        case ERR_EMPTY_PREDICATE:
        case ERR_UNBALANCED_PRED_DELIM:
        case ERR_EXTRA_JUNK_AFTER_PREDICATE:
        case ERR_UNSUPPORTED_PRED_TYPE:
        case ERR_EXPECTED_INTEGER:
        case ERR_INVALID_NUMBER:
            asprintf(&message, MESSAGES[path->result.code], path->result.position + 1);
            break;
        case ERR_UNEXPECTED_VALUE:
            asprintf(&message, MESSAGES[path->result.code], path->result.position + 1, path->result.actual_char, path->result.expected_char);
            break;
        case ERR_EXPECTED_NAME_CHAR:
            asprintf(&message, MESSAGES[path->result.code], path->result.position + 1, path->result.actual_char);
            break;
        default:
            message = make_simple_status_message(path->result.code);
            break;
    }

    return message;
}

static char *make_simple_status_message(jsonpath_status_code code)
{
    char *message = NULL;
    
    size_t len = strlen(MESSAGES[code]) + 1;
    message = (char *)malloc(len);
    if(NULL != message)
    {
        memcpy(message, (void *)MESSAGES[code], len);
    }

    return message;
}

