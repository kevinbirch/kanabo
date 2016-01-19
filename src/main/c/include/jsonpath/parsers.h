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


#include <stdint.h>

#include "jsonpath/input.h"

enum parser_result_code
{
    PARSER_SUCCESS = 0,
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
    ERR_NO_ALTERNATIVE,              // none of the alternatives matched the input
};

typedef enum parser_result_code parser_result_code;

typedef struct parser_s Parser;


/* Non-terminal parsers */

Parser *rule_parser(const char *name, Parser *expression);
#define rule(PARSER) rule_parser(__func__, (PARSER))

Parser *choice_parser(Parser *one, Parser *two, ...);
#define choice(...) choice_parser(__VA_ARGS__, NULL)

Parser *sequence_parser(Parser *one, Parser *two, ...);
#define sequence(...) sequence_parser(__VA_ARGS__, NULL)

Parser *option(Parser *optional);
Parser *repetition(Parser *repeated);

/* Terminal parsers */

Parser *literal(const char *value);

Parser *number(void);
Parser *integer(void);
Parser *signed_integer(void);
Parser *non_zero_signed_integer(void);

Parser *quoted_string(void);
Parser *string(void);

/* Destructor */

void parser_free(Parser *value);

/* Parser Execution */

char *parser_status_message(parser_result_code code, uint8_t argument, Input *input);
