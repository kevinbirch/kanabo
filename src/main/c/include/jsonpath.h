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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// jsonpath model entities

enum path_kind
{
    ABSOLUTE_PATH,
    RELATIVE_PATH
};

typedef struct jsonpath jsonpath;

enum step_kind
{
    ROOT,
    SINGLE,
    RECURSIVE
};

enum test_kind
{
    WILDCARD_TEST,
    NAME_TEST,
    TYPE_TEST
};

enum type_test_kind
{
    OBJECT_TEST,
    ARRAY_TEST,
    STRING_TEST,
    NUMBER_TEST,
    BOOLEAN_TEST,
    NULL_TEST,
};
typedef struct step step;

enum predicate_kind
{
    WILDCARD,
    SUBSCRIPT,
    SLICE,
    JOIN
};
typedef struct predicate predicate;

enum parser_status_code
{
    JSONPATH_SUCCESS = 0,
    ERR_NULL_EXPRESSION,             // the expression argument given was NULL
    ERR_ZERO_LENGTH,                 // expression length was 0
    ERR_PARSER_OUT_OF_MEMORY,        // unable to allocate memory
    ERR_NOT_JSONPATH,                // not a JSONPath expression
    ERR_PREMATURE_END_OF_INPUT,      // premature end of input before expression was complete
    ERR_UNEXPECTED_VALUE,            // expected one character but found another
    ERR_EMPTY_PREDICATE,             // a predicate is empty
    ERR_UNBALANCED_PRED_DELIM,       // missing closing predicate delimiter `]' before end of step
    ERR_UNSUPPORTED_PRED_TYPE,       // unsupported predicate found
    ERR_EXTRA_JUNK_AFTER_PREDICATE,  // extra characters after valid predicate definition
    ERR_EXPECTED_NAME_CHAR,          // expected a name character, but found something else instead
    ERR_EXPECTED_NODE_TYPE_TEST,     // expected a node type test
    ERR_EXPECTED_INTEGER,            // expected an integer
    ERR_INVALID_NUMBER,              // invalid number
    ERR_STEP_CANNOT_BE_ZERO,         // slice step value must be non-zero
};

typedef enum parser_status_code parser_status_code;

typedef struct parser_context parser_context;

// jsonpath parser api
parser_context *make_parser(const uint8_t *expression, size_t length);
parser_status_code parser_status(const parser_context * restrict context);

void parser_free(parser_context *context);

jsonpath *parse(parser_context *context);
void path_free(jsonpath *path);

char *parser_status_message(const parser_context * restrict context);

// jsonpath model api

typedef bool (*path_iterator)(step *each, void *context);
bool path_iterate(const jsonpath * restrict path, path_iterator iterator, void *context);

enum path_kind path_kind(const jsonpath * restrict path);
const char *   path_kind_name(enum path_kind value);
size_t         path_length(const jsonpath * restrict path);
step *         path_get(const jsonpath * restrict path, size_t index);

enum step_kind step_kind(const step * restrict value);
const char *   step_kind_name(enum step_kind value);
enum test_kind step_test_kind(const step * restrict value);
const char *   test_kind_name(enum test_kind value);

enum type_test_kind type_test_step_kind(const step * restrict value);
const char *        type_test_kind_name(enum type_test_kind value);
uint8_t *           name_test_step_name(const step * restrict value);
size_t              name_test_step_length(const step * restrict value);

bool       step_has_predicate(const step * restrict value);
predicate *step_predicate(const step * restrict value);

enum predicate_kind predicate_kind(const predicate * restrict value);
const char *        predicate_kind_name(enum predicate_kind value);
size_t              subscript_predicate_index(const predicate * restrict value);

int_fast32_t        slice_predicate_to(const predicate * restrict value);
int_fast32_t        slice_predicate_from(const predicate * restrict value);
int_fast32_t        slice_predicate_step(const predicate * restrict value);
bool                slice_predicate_has_to(const predicate * restrict value);
bool                slice_predicate_has_from(const predicate * restrict value);
bool                slice_predicate_has_step(const predicate * restrict value);

jsonpath *          join_predicate_left(const predicate * restrict value);
jsonpath *          join_predicate_right(const predicate * restrict value);

