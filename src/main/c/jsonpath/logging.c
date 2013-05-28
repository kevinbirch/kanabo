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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include "jsonpath.h"
#include "jsonpath/private.h"
#include "conditions.h"

static const char * const MESSAGES[] = 
{
    "Success.",
    "Expression argument was NULL.",
    "Expression length was 0.",
    "Unable to allocate memory.",
    "Not a JSONPath expression.",
    "Premature end of input after position %d.",
    "At position %d: unexpected character '%c', was expecting '%c' instead.",
    "At position %d: empty predicate.",
    "At position %d: missing closing predicate delimiter `]' before end of step.",
    "At position %d: unsupported predicate found.",
    "At position %d: extra characters after valid predicate definition.",
    "At position %d: expected a name character, but found '%c' instead.",
    "At position %d: expected a node type test.",
    "At position %d: expected an integer.",
    "At position %d: invalid number.",
    "At position %d: slice step value must be non-zero."
};

static const char * const PATH_KIND_NAMES[] =
{
    "absolute path",
    "relative path"
};

static const char * const STEP_KIND_NAMES[] =
{
    "root step",
    "single step",
    "recursive step"
};

static const char * const TEST_KIND_NAMES[] =
{
    "wildcard test",
    "name test",
    "type test"
};

static const char * const PREDICATE_KIND_NAMES[] =
{
    "wildcard predicate",
    "subscript predicate",
    "slice predicate",
    "join predicate"
};

static const char * const TYPE_TEST_KIND_NAMES[] =
{
    "object test",
    "array test",
    "string test",
    "number test",
    "boolean test",
    "null test"
};


const char *path_kind_name(enum path_kind value)
{
    return PATH_KIND_NAMES[value];
}

const char *step_kind_name(enum step_kind value)
{
    return STEP_KIND_NAMES[value];
}

const char *test_kind_name(enum test_kind value)
{
    return TEST_KIND_NAMES[value];
}

const char *type_test_kind_name(enum type_test_kind value)
{
    return TYPE_TEST_KIND_NAMES[value];
}

const char *predicate_kind_name(enum predicate_kind value)
{
    return PREDICATE_KIND_NAMES[value];
}

char *parser_status_message(const parser_context * restrict context)
{
    PRECOND_NONNULL_ELSE_NULL(context);
    char *message = NULL;
    int result = 0;
    
    switch(context->result.code)
    {
        case ERR_PREMATURE_END_OF_INPUT:
            result = asprintf(&message, MESSAGES[context->result.code], context->cursor);
            break;
        case ERR_EXPECTED_NODE_TYPE_TEST:
        case ERR_EMPTY_PREDICATE:
        case ERR_UNBALANCED_PRED_DELIM:
        case ERR_EXTRA_JUNK_AFTER_PREDICATE:
        case ERR_UNSUPPORTED_PRED_TYPE:
        case ERR_EXPECTED_INTEGER:
        case ERR_INVALID_NUMBER:
            result = asprintf(&message, MESSAGES[context->result.code], context->cursor + 1);
            break;
        case ERR_UNEXPECTED_VALUE:
            result = asprintf(&message, MESSAGES[context->result.code], context->cursor + 1, context->result.actual_char, context->result.expected_char);
            break;
        case ERR_EXPECTED_NAME_CHAR:
            result = asprintf(&message, MESSAGES[context->result.code], context->cursor + 1, context->result.actual_char);
            break;
        default:
            message = strdup(MESSAGES[context->result.code]);
            break;
    }
    if(-1 == result)
    {
        message = NULL;
    }

    return message;
}

