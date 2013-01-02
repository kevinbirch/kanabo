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

#include <check.h>

#include "jsonpath.h"
#include "test.h"

static void assert_parser_result(jsonpath_status_code result, jsonpath *path, enum path_kind expected_kind, size_t expected_length);

static void assert_root_step(jsonpath *path);
static void assert_single_name_step(jsonpath *path, size_t index, char *name);
static void assert_recursive_name_step(jsonpath *path, size_t index, char *name);
static void assert_name_step(jsonpath *path, size_t index, char *name, enum step_kind expected_step_kind);
static void assert_single_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind);
static void assert_recursive_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind);
static void assert_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind, enum step_kind expected_step_kind);
static void assert_step(jsonpath *path, size_t index, enum step_kind expected_step_kind, enum test_kind expected_test_kind);
static void assert_name(step * step, uint8_t *value, size_t length);
static void assert_no_predicates(jsonpath *path, size_t index);
static void assert_wildcard_predicate(jsonpath *path, size_t path_index, size_t predicate_index);
static void assert_subscript_predicate(jsonpath *path, size_t path_index, size_t predicate_index, uint_fast32_t index_value);
static void assert_predicate(jsonpath *path, size_t path_index, size_t predicate_index, enum predicate_kind expected_predicate_kind);

START_TEST (null_expression)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath(NULL, 50, &path);
    
    ck_assert_int_eq(ERR_NULL_EXPRESSION, result);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    ck_assert_int_eq(0, path.result.position);
}
END_TEST

START_TEST (zero_length)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"", 0, &path);
    
    ck_assert_int_eq(ERR_ZERO_LENGTH, result);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    ck_assert_int_eq(0, path.result.position);
}
END_TEST

START_TEST (null_path)
{
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"", 5, NULL);
    
    ck_assert_int_eq(ERR_NULL_OUTPUT_PATH, result);
}
END_TEST

START_TEST (missing_step_test)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$.", 2, &path);
    
    ck_assert_int_eq(ERR_PREMATURE_END_OF_INPUT, result);
    ck_assert_int_eq(2, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (missing_recursive_step_test)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$..", 3, &path);
    
    ck_assert_int_eq(ERR_PREMATURE_END_OF_INPUT, result);
    ck_assert_int_eq(3, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (missing_dot)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$x", 2, &path);
    
    ck_assert_int_eq(ERR_UNEXPECTED_VALUE, result);
    ck_assert_int_eq(1, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (relative_path_begins_with_dot)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)".x", 2, &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NAME_CHAR, result);
    ck_assert_int_eq(0, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (quoted_empty_step)
{
    jsonpath path;
    char *expression = "$.foo.''.bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NAME_CHAR, result);
    ck_assert_int_eq(7, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (empty_predicate)
{
    jsonpath path;
    char *expression = "$.foo[].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EMPTY_PREDICATE, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (extra_junk_in_predicate)
{
    jsonpath path;
    char *expression = "$.foo[ * quux].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXTRA_JUNK_AFTER_PREDICATE, result);
    ck_assert_int_eq(9, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (whitespace_predicate)
{
    jsonpath path;
    char *expression = "$.foo[ \t ].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EMPTY_PREDICATE, result);
    ck_assert_int_eq(9, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_predicate)
{
    jsonpath path;
    char *expression = "$.foo[asdf].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_UNSUPPORTED_PRED_TYPE, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_type_test_name)
{
    jsonpath path;
    char *expression = "$.foo.monkey()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_type_test_name_oblong)
{
    jsonpath path;
    char *expression = "$.foo.oblong()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_type_test_name_alloy)
{
    jsonpath path;
    char *expression = "$.foo.alloy()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_type_test_name_strong)
{
    jsonpath path;
    char *expression = "$.foo.strong()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_type_test_name_numred)
{
    jsonpath path;
    char *expression = "$.foo.numred()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_type_test_name_booloud)
{
    jsonpath path;
    char *expression = "$.foo.booloud()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (bogus_type_test_name_narl)
{
    jsonpath path;
    char *expression = "$.foo.narl()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

START_TEST (empty_type_test_name)
{
    jsonpath path;
    char *expression = "$.foo.()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    fprintf(stdout, "received expected failure message: '%s'\n", message);

    free(message);
    free_jsonpath(&path);
}
END_TEST

static void assert_parser_result(jsonpath_status_code result, jsonpath *path, enum path_kind expected_kind, size_t expected_length)
{
    ck_assert_int_eq(SUCCESS, result);
    char *message = make_status_message(path);
    ck_assert_not_null(message);
    free(message);
    ck_assert_int_ne(0, path->result.position);
    ck_assert_int_eq(expected_kind, path_get_kind(path));
    ck_assert_not_null(path->steps);
    ck_assert_int_eq(expected_length, path_get_length(path));
}

static void assert_root_step(jsonpath *path)
{
    assert_step(path, 0, ROOT, NAME_TEST);
    assert_no_predicates(path, 0);
}

static void assert_single_name_step(jsonpath *path, size_t index, char *name)
{
    assert_name_step(path, index, name, SINGLE);
}

static void assert_recursive_name_step(jsonpath *path, size_t index, char *name)
{
    assert_name_step(path, index, name, RECURSIVE);
}

static void assert_name_step(jsonpath *path, size_t index, char *name, enum step_kind expected_step_kind)
{
    assert_step(path, index, expected_step_kind, NAME_TEST);
    assert_name(path_get_step(path, index), (uint8_t *)name, strlen(name));
}

static void assert_single_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind)
{
    assert_type_step(path, index, expected_type_kind, SINGLE);
}

static void assert_recursive_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind)
{
    assert_type_step(path, index, expected_type_kind, RECURSIVE);
}

static void assert_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind, enum step_kind expected_step_kind)
{
    assert_step(path, index, expected_step_kind, TYPE_TEST);
    ck_assert_int_eq(expected_type_kind, type_test_step_get_type(path_get_step(path, index)));
}

static void assert_step(jsonpath *path, size_t index, enum step_kind expected_step_kind, enum test_kind expected_test_kind)
{
    ck_assert_int_eq(expected_step_kind, step_get_kind(path_get_step(path, index)));
    ck_assert_int_eq(expected_test_kind, step_get_test_kind(path_get_step(path, index)));
}

static void assert_name(step *step, uint8_t *name, size_t length)
{
    ck_assert_int_eq(length, name_test_step_get_length(step));
    ck_assert_buf_eq(name, length, name_test_step_get_name(step), name_test_step_get_length(step));
}

static void assert_no_predicates(jsonpath *path, size_t index)
{
    ck_assert_int_eq(0, step_get_predicate_count(path_get_step(path, index)));
    ck_assert_null(path->steps[index]->predicates);
}

static void assert_wildcard_predicate(jsonpath *path, size_t path_index, size_t predicate_index)
{
    assert_predicate(path, path_index, predicate_index, WILDCARD);
}

static void assert_subscript_predicate(jsonpath *path, size_t path_index, size_t predicate_index, uint_fast32_t index_value)
{
    assert_predicate(path, path_index, predicate_index, SUBSCRIPT);
    ck_assert_int_eq(index_value, subscript_predicate_get_index(step_get_predicate(path_get_step(path, path_index), predicate_index)));
}

static void assert_predicate(jsonpath *path, size_t path_index, size_t predicate_index, enum predicate_kind expected_predicate_kind)
{
    ck_assert_int_ne(0, step_get_predicate_count(path_get_step(path, path_index)));
    ck_assert_not_null(path->steps[path_index]->predicates);
    ck_assert_not_null(step_get_predicate(path_get_step(path, path_index), predicate_index));
    ck_assert_int_eq(expected_predicate_kind, predicate_get_kind(step_get_predicate(path_get_step(path, path_index), predicate_index)));
}

START_TEST (dollar_only)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$", 1, &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 1);
    assert_root_step(&path);

    free_jsonpath(&path);
}
END_TEST

START_TEST (absolute_single_step)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$.foo", 5, &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 2);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_no_predicates(&path, 0);

    free_jsonpath(&path);    
}
END_TEST

START_TEST (absolute_recursive_step)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$..foo", 6, &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 2);
    assert_root_step(&path);
    assert_recursive_name_step(&path, 1, "foo");
    assert_no_predicates(&path, 0);

    free_jsonpath(&path);    
}
END_TEST

START_TEST (absolute_multi_step)
{
    jsonpath path;
    char *expression = "$.foo.baz..yobble.thingum";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 5);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_name_step(&path, 2, "baz");
    assert_recursive_name_step(&path, 3, "yobble");
    assert_single_name_step(&path, 4, "thingum");
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);
    assert_no_predicates(&path, 3);
    assert_no_predicates(&path, 4);

    free_jsonpath(&path);    
}
END_TEST

START_TEST (relative_multi_step)
{
    jsonpath path;
    char *expression = "foo.bar..baz";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, RELATIVE_PATH, 3);
    assert_single_name_step(&path, 0, "foo");
    assert_single_name_step(&path, 1, "bar");
    assert_recursive_name_step(&path, 2, "baz");
    assert_no_predicates(&path, 0);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);    
}
END_TEST

START_TEST (quoted_multi_step)
{
    jsonpath path;
    char *expression = "$.foo.'happy fun ball'.bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 4);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_name_step(&path, 2, "happy fun ball");
    assert_single_name_step(&path, 3, "bar");
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);
    assert_no_predicates(&path, 3);

    free_jsonpath(&path);    
}
END_TEST

START_TEST (whitespace)
{
    jsonpath path;
    char *expression = "  $ \r\n. foo \n.\n. \t'happy fun ball' . \t string()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 4);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_recursive_name_step(&path, 2, "happy fun ball");
    assert_single_type_step(&path, 3, STRING_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);
    assert_no_predicates(&path, 3);

    free_jsonpath(&path);    
}
END_TEST

START_TEST (type_test_missing_closing_paren)
{
    jsonpath path;
    char *expression = "$.foo.null(";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_name_step(&path, 2, "null(");
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (type_test_with_trailing_junk)
{
    jsonpath path;
    char *expression = "$.foo.array()[0]";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, ARRAY_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (recursive_type_test)
{
    jsonpath path;
    char *expression = "$.foo..string()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_recursive_type_step(&path, 2, STRING_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (object_type_test)
{
    jsonpath path;
    char *expression = "$.foo.object()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, OBJECT_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (array_type_test)
{
    jsonpath path;
    char *expression = "$.foo.array()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, ARRAY_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (string_type_test)
{
    jsonpath path;
    char *expression = "$.foo.string()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, STRING_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (number_type_test)
{
    jsonpath path;
    char *expression = "$.foo.number()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, NUMBER_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (boolean_type_test)
{
    jsonpath path;
    char *expression = "$.foo.boolean()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, BOOLEAN_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (null_type_test)
{
    jsonpath path;
    char *expression = "$.foo.null()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, NULL_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (wildcard_predicate)
{
    jsonpath path;
    char *expression = "$.store.book[*].author";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 4);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "store");
    assert_no_predicates(&path, 1);
    assert_single_name_step(&path, 2, "book");
    assert_wildcard_predicate(&path, 2, 0);
    assert_single_name_step(&path, 3, "author");
    assert_no_predicates(&path, 3);

    free_jsonpath(&path);
}
END_TEST

START_TEST (wildcard_predicate_with_whitespace)
{
    jsonpath path;
    char *expression = "$.foo  [\t*\n]  .bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_wildcard_predicate(&path, 1, 0);
    assert_single_name_step(&path, 2, "bar");
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

START_TEST (subscript_predicate)
{
    jsonpath path;
    char *expression = "$.foo[42].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_subscript_predicate(&path, 1, 0, 42);
    assert_single_name_step(&path, 2, "bar");
    assert_no_predicates(&path, 2);

    free_jsonpath(&path);
}
END_TEST

Suite *jsonpath_suite(void)
{
    TCase *bad_input = tcase_create("bad input");
    tcase_add_test(bad_input, null_expression);
    tcase_add_test(bad_input, zero_length);
    tcase_add_test(bad_input, null_path);
    tcase_add_test(bad_input, missing_step_test);
    tcase_add_test(bad_input, missing_recursive_step_test);
    tcase_add_test(bad_input, missing_dot);
    tcase_add_test(bad_input, relative_path_begins_with_dot);
    tcase_add_test(bad_input, quoted_empty_step);
    tcase_add_test(bad_input, bogus_type_test_name);
    tcase_add_test(bad_input, bogus_type_test_name_oblong);
    tcase_add_test(bad_input, bogus_type_test_name_alloy);
    tcase_add_test(bad_input, bogus_type_test_name_strong);
    tcase_add_test(bad_input, bogus_type_test_name_numred);
    tcase_add_test(bad_input, bogus_type_test_name_booloud);
    tcase_add_test(bad_input, bogus_type_test_name_narl);
    tcase_add_test(bad_input, empty_type_test_name);
    tcase_add_test(bad_input, empty_predicate);
    tcase_add_test(bad_input, whitespace_predicate);
    tcase_add_test(bad_input, extra_junk_in_predicate);
    tcase_add_test(bad_input, bogus_predicate);

    TCase *basic = tcase_create("basic");
    tcase_add_test(basic, dollar_only);
    tcase_add_test(basic, absolute_single_step);
    tcase_add_test(basic, absolute_recursive_step);
    tcase_add_test(basic, absolute_multi_step);
    tcase_add_test(basic, quoted_multi_step);
    tcase_add_test(basic, relative_multi_step);
    tcase_add_test(basic, whitespace);

    TCase *node_type = tcase_create("node type test");
    tcase_add_test(node_type, type_test_missing_closing_paren);
    tcase_add_test(node_type, type_test_with_trailing_junk);
    tcase_add_test(node_type, recursive_type_test);
    tcase_add_test(node_type, object_type_test);
    tcase_add_test(node_type, array_type_test);
    tcase_add_test(node_type, string_type_test);
    tcase_add_test(node_type, number_type_test);
    tcase_add_test(node_type, boolean_type_test);
    tcase_add_test(node_type, null_type_test);

    TCase *predicate = tcase_create("predicate");
    tcase_add_test(predicate, wildcard_predicate);
    tcase_add_test(predicate, wildcard_predicate_with_whitespace);
    tcase_add_test(predicate, subscript_predicate);
    
    Suite *jsonpath = suite_create("JSONPath");
    suite_add_tcase(jsonpath, bad_input);
    suite_add_tcase(jsonpath, basic);
    suite_add_tcase(jsonpath, node_type);
    suite_add_tcase(jsonpath, predicate);

    return jsonpath;
}
