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

#ifndef JSONPATH_H
#define JSONPATH_H

#include <stdlib.h>
#include <stdint.h>

typedef struct jsonpath jsonpath;

struct predicate
{
    enum predicate_kind
    {
        WILDCARD,
        SUBSCRIPT,
        SLICE,
        JOIN
    } kind;
    
    union
    {
        struct
        {
            uint_fast32_t index;
        } subscript;
        
        struct
        {
            uint_fast32_t from;
            uint_fast32_t to;
            uint_fast32_t step;
        } slice;
        
        struct
        {
            jsonpath *left;
            jsonpath *right;
        } join;
    };
};

typedef struct predicate predicate;

struct step
{
    enum step_kind
    {
        ROOT,
        SINGLE,
        RECURSIVE
    } kind;
    
    struct
    {
        enum test_kind
        {
            NAME_TEST,
            TYPE_TEST
        } kind;

        union
        {
            struct
            {
                uint8_t *value;
                size_t  length;
            } name;
        
            enum type_test_kind
            {
                OBJECT_TEST,
                ARRAY_TEST,
                STRING_TEST,
                NUMBER_TEST,
                BOOLEAN_TEST,
                NULL_TEST
            } type;
        };
    } test;    

    size_t    predicate_count;
    predicate **predicates;
};

typedef struct step step;

enum jsonpath_status_code
{
    JSONPATH_SUCCESS = 0,
    ERR_NULL_EXPRESSION,
    ERR_ZERO_LENGTH,
    ERR_NULL_OUTPUT_PATH,
    ERR_OUT_OF_MEMORY,
    ERR_NOT_JSONPATH,
    ERR_PREMATURE_END_OF_INPUT,
    ERR_UNEXPECTED_VALUE,
    ERR_EMPTY_PREDICATE,
    ERR_UNBALANCED_PRED_DELIM,
    ERR_UNSUPPORTED_PRED_TYPE,
    ERR_EXTRA_JUNK_AFTER_PREDICATE,
    ERR_EXPECTED_NAME_CHAR,
    ERR_EXPECTED_NODE_TYPE_TEST,
    ERR_EXPECTED_INTEGER,
    ERR_INVALID_NUMBER
};

typedef enum jsonpath_status_code jsonpath_status_code;

struct jsonpath
{
    enum path_kind
    {
        ABSOLUTE_PATH,
        RELATIVE_PATH
    } kind;
  
    size_t length;
    step **steps;

    struct
    {
        jsonpath_status_code code;
        size_t position;
        uint8_t expected_char;
        uint8_t actual_char;
    } result;
};

// parser entry point
jsonpath_status_code parse_jsonpath(const uint8_t *expression, size_t length, jsonpath *path);
char *make_status_message(const jsonpath * restrict path);

void jsonpath_free(jsonpath *path);

// jsonpath model api
enum path_kind path_get_kind(const jsonpath * restrict path);
size_t         path_get_length(const jsonpath * restrict path);
step *         path_get_step(const jsonpath * restrict path, size_t index);

enum step_kind step_get_kind(const step * restrict step);
enum test_kind step_get_test_kind(const step * restrict step);

enum type_test_kind type_test_step_get_type(const step * restrict step);
uint8_t            *name_test_step_get_name(const step * restrict step);
size_t              name_test_step_get_length(const step * restrict step);

size_t     step_get_predicate_count(const step * restrict step);
predicate *step_get_predicate(const step * restrict step, size_t index);

enum predicate_kind predicate_get_kind(const predicate * restrict predicate);
uint_fast32_t       subscript_predicate_get_index(const predicate * restrict predicate);
uint_fast32_t       slice_predicate_get_to(const predicate * restrict predicate);
uint_fast32_t       slice_predicate_get_from(const predicate * restrict predicate);
uint_fast32_t       slice_predicate_get_step(const predicate * restrict predicate);
jsonpath           *join_predicate_get_left(const predicate * restrict predicate);
jsonpath           *join_predicate_get_right(const predicate * restrict predicate);

#endif
