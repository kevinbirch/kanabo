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

 /* JSONPath Entities */

typedef struct jsonpath jsonpath;
typedef struct step step;
typedef struct predicate predicate;

enum path_kind
{
    ABSOLUTE_PATH,
    RELATIVE_PATH
};

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

enum predicate_kind
{
    WILDCARD,
    SUBSCRIPT,
    SLICE,
    JOIN
};

 /* JSONPath Parser Api */

struct maybe_jsonpath_s
{
    enum
    {
        ERROR,
        JSONPATH,
    } tag;

    union
    {
        struct
        {
            char   *message;
            size_t  position;
        } error;
        jsonpath *path;
    };
};

typedef struct maybe_jsonpath_s MaybeJsonpath;

MaybeJsonpath parse(const uint8_t *expression, size_t length);
void          maybe_jsonpath_free(MaybeJsonpath result);
void          jsonpath_free(jsonpath *path);


/* Path API */
enum path_kind      path_kind(const jsonpath *path);
const char *        path_kind_name(enum path_kind value);
size_t              path_length(const jsonpath *path);
step *              path_get(const jsonpath *path, size_t index);

typedef bool (*path_iterator)(step *each, void *parser);
bool path_iterate(const jsonpath *path, path_iterator iterator, void *context);

/* Step API */
enum step_kind      step_kind(const step *value);
const char *        step_kind_name(enum step_kind value);
enum test_kind      step_test_kind(const step *value);
const char *        test_kind_name(enum test_kind value);

/* Type Test API */
enum type_test_kind type_test_step_kind(const step *value);
const char *        type_test_kind_name(enum type_test_kind value);

/* Name Test API */
uint8_t *           name_test_step_name(const step *value);
size_t              name_test_step_length(const step *value);

/* Predicate API */
bool                step_has_predicate(const step *value);
predicate *         step_predicate(const step *value);

enum predicate_kind predicate_kind(const predicate *value);
const char *        predicate_kind_name(enum predicate_kind value);

/* Subscript Predicate API */
size_t              subscript_predicate_index(const predicate *value);

/* Slice Predicate API */
int_fast32_t        slice_predicate_to(const predicate *value);
int_fast32_t        slice_predicate_from(const predicate *value);
int_fast32_t        slice_predicate_step(const predicate *value);
bool                slice_predicate_has_to(const predicate *value);
bool                slice_predicate_has_from(const predicate *value);
bool                slice_predicate_has_step(const predicate *value);

/* Join (Union) PredicateAPI */
jsonpath *          join_predicate_left(const predicate *value);
jsonpath *          join_predicate_right(const predicate *value);

