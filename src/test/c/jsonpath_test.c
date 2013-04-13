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
#include <errno.h>

#include <check.h>

#include "jsonpath.h"
#include "test.h"
#include "log.h"

static void assert_parser_result(const char * restrict expression, jsonpath_status_code result, jsonpath *path, enum path_kind expected_kind, size_t expected_length);

static void assert_root_step(jsonpath *path);
static void assert_single_name_step(jsonpath *path, size_t index, char *name);
static void assert_single_wildcard_step(jsonpath *path, size_t index);
static void assert_recursive_wildcard_step(jsonpath *path, size_t index);
static void assert_wildcard_step(jsonpath *path, size_t index, enum step_kind expected_step_kind);
static void assert_recursive_name_step(jsonpath *path, size_t index, char *name);
static void assert_name_step(jsonpath *path, size_t index, char *name, enum step_kind expected_step_kind);
static void assert_single_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind);
static void assert_recursive_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind);
static void assert_type_step(jsonpath *path, size_t index, enum type_test_kind expected_type_kind, enum step_kind expected_step_kind);
static void assert_step(jsonpath *path, size_t index, enum step_kind expected_step_kind, enum test_kind expected_test_kind);
static void assert_name(step * current, uint8_t *value, size_t length);
static void assert_no_predicates(jsonpath *path, size_t index);
static void assert_wildcard_predicate(jsonpath *path, size_t path_index);
static void assert_subscript_predicate(jsonpath *path, size_t path_index, uint_fast32_t index_value);
static void assert_predicate(jsonpath *path, size_t path_index, enum predicate_kind expected_predicate_kind);

START_TEST (null_expression)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath(NULL, 50, &path);
    
    ck_assert_int_eq(ERR_NULL_EXPRESSION, result);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

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

    log_info("parser test", "received expected failure message: '%s'", message);

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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (bogus_type_test_name_numbered)
{
    jsonpath path;
    char *expression = "$.foo.numbered()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (bogus_type_test_name_booger)
{
    jsonpath path;
    char *expression = "$.foo.booger()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    ck_assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
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

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

static void assert_parser_result(const char * restrict expression, jsonpath_status_code result, jsonpath *path, enum path_kind expected_kind, size_t expected_length)
{
    char *message = make_status_message(path);
    ck_assert_not_null(message);
    if(JSONPATH_SUCCESS != result)
    {
        log_error("parser test", "failed for the expression: '%s', received: '%s'", expression, message);
    }
    free(message);
    ck_assert_int_eq(JSONPATH_SUCCESS, result);
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

static void assert_single_wildcard_step(jsonpath *path, size_t index)
{
    assert_wildcard_step(path, index, SINGLE);
}

static void assert_recursive_wildcard_step(jsonpath *path, size_t index)
{
    assert_wildcard_step(path, index, RECURSIVE);
}

static void assert_wildcard_step(jsonpath *path, size_t index, enum step_kind expected_step_kind)
{
    assert_step(path, index, expected_step_kind, WILDCARD_TEST);
    assert_name(path_get_step(path, index), (uint8_t *)"*", 1);
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

static void assert_name(step *current, uint8_t *name, size_t length)
{
    ck_assert_int_eq(length, name_test_step_get_length(current));
    ck_assert_buf_eq(name, length, name_test_step_get_name(current), name_test_step_get_length(current));
}

static void assert_no_predicates(jsonpath *path, size_t index)
{
    ck_assert_false(step_has_predicate(path_get_step(path, index)));
    ck_assert_null(path->steps[index]->predicate);
}

static void assert_wildcard_predicate(jsonpath *path, size_t path_index)
{
    assert_predicate(path, path_index, WILDCARD);
}

static void assert_subscript_predicate(jsonpath *path, size_t path_index, uint_fast32_t index_value)
{
    assert_predicate(path, path_index, SUBSCRIPT);
    ck_assert_int_eq(index_value, subscript_predicate_get_index(step_get_predicate(path_get_step(path, path_index))));
}

static void assert_predicate(jsonpath *path, size_t path_index, enum predicate_kind expected_predicate_kind)
{
    ck_assert_true(step_has_predicate(path_get_step(path, path_index)));
    ck_assert_not_null(path->steps[path_index]->predicate);
    ck_assert_not_null(step_get_predicate(path_get_step(path, path_index)));
    ck_assert_int_eq(expected_predicate_kind, predicate_get_kind(step_get_predicate(path_get_step(path, path_index))));
}

START_TEST (dollar_only)
{
    jsonpath path;
    char *expression = "$";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, 1, &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 1);
    assert_root_step(&path);

    jsonpath_free(&path);
}
END_TEST

START_TEST (absolute_single_step)
{
    jsonpath path;
    char *expression = "$.foo";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, 5, &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 2);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_no_predicates(&path, 0);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (absolute_recursive_step)
{
    jsonpath path;
    char *expression = "$..foo";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, 6, &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 2);
    assert_root_step(&path);
    assert_recursive_name_step(&path, 1, "foo");
    assert_no_predicates(&path, 0);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (absolute_multi_step)
{
    jsonpath path;
    char *expression = "$.foo.baz..yobble.thingum";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 5);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_name_step(&path, 2, "baz");
    assert_recursive_name_step(&path, 3, "yobble");
    assert_single_name_step(&path, 4, "thingum");
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);
    assert_no_predicates(&path, 3);
    assert_no_predicates(&path, 4);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (relative_multi_step)
{
    jsonpath path;
    char *expression = "foo.bar..baz";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, RELATIVE_PATH, 3);
    assert_single_name_step(&path, 0, "foo");
    assert_single_name_step(&path, 1, "bar");
    assert_recursive_name_step(&path, 2, "baz");
    assert_no_predicates(&path, 0);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (quoted_multi_step)
{
    jsonpath path;
    char *expression = "$.foo.'happy fun ball'.bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 4);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_name_step(&path, 2, "happy fun ball");
    assert_single_name_step(&path, 3, "bar");
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);
    assert_no_predicates(&path, 3);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (wildcard)
{
    jsonpath path;
    char *expression = "$.foo.*";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_wildcard_step(&path, 2);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (recursive_wildcard)
{
    jsonpath path;
    char *expression = "$.foo..*";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_recursive_wildcard_step(&path, 2);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_with_trailing_junk)
{
    jsonpath path;
    char *expression = "$.foo.* [0]";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXTRA_JUNK_AFTER_WILDCARD, result);
    ck_assert_int_eq(7, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (whitespace)
{
    jsonpath path;
    char *expression = "  $ \r\n. foo \n.\n. \t'happy fun ball' . \t string()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 4);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_recursive_name_step(&path, 2, "happy fun ball");
    assert_single_type_step(&path, 3, STRING_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);
    assert_no_predicates(&path, 3);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (type_test_missing_closing_paren)
{
    jsonpath path;
    char *expression = "$.foo.null(";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_name_step(&path, 2, "null(");
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (type_test_with_trailing_junk)
{
    jsonpath path;
    char *expression = "$.foo.array()[0]";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    ck_assert_int_eq(ERR_EXTRA_JUNK_AFTER_TYPE_TEST, result);
    ck_assert_int_eq(13, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (recursive_type_test)
{
    jsonpath path;
    char *expression = "$.foo..string()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_recursive_type_step(&path, 2, STRING_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (object_type_test)
{
    jsonpath path;
    char *expression = "$.foo.object()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, OBJECT_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (array_type_test)
{
    jsonpath path;
    char *expression = "$.foo.array()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, ARRAY_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (string_type_test)
{
    jsonpath path;
    char *expression = "$.foo.string()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, STRING_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (number_type_test)
{
    jsonpath path;
    char *expression = "$.foo.number()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, NUMBER_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (boolean_type_test)
{
    jsonpath path;
    char *expression = "$.foo.boolean()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, BOOLEAN_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (null_type_test)
{
    jsonpath path;
    char *expression = "$.foo.null()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_single_type_step(&path, 2, NULL_TEST);
    assert_no_predicates(&path, 1);
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_predicate)
{
    jsonpath path;
    char *expression = "$.store.book[*].author";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 4);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "store");
    assert_no_predicates(&path, 1);
    assert_single_name_step(&path, 2, "book");
    assert_wildcard_predicate(&path, 2);
    assert_single_name_step(&path, 3, "author");
    assert_no_predicates(&path, 3);

    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_predicate_with_whitespace)
{
    jsonpath path;
    char *expression = "$.foo  [\t*\n]  .bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_wildcard_predicate(&path, 1);
    assert_single_name_step(&path, 2, "bar");
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (subscript_predicate)
{
    jsonpath path;
    char *expression = "$.foo[42].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_subscript_predicate(&path, 1, 42);
    assert_single_name_step(&path, 2, "bar");
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (subscript_predicate_with_whitespace)
{
    jsonpath path;
    char *expression = "$.foo  [\t42\r]\n.bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);
    assert_root_step(&path);
    assert_single_name_step(&path, 1, "foo");
    assert_subscript_predicate(&path, 1, 42);
    assert_single_name_step(&path, 2, "bar");
    assert_no_predicates(&path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (negative_subscript_predicate)
{
    jsonpath path;
    char *expression = "$.foo[ -3].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    // xxx - fixme! this should be ERR_EXPECTED_INTEGER instead!
    ck_assert_int_eq(ERR_UNSUPPORTED_PRED_TYPE, result);
    ck_assert_int_eq(7, path.result.position);
    char *message = make_status_message(&path);
    ck_assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);

    jsonpath_free(&path);
}
END_TEST

START_TEST (bad_path_input)
{
    errno = 0;
    ck_assert_int_eq(-1, path_get_kind(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(0, path_get_length(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(path_get_step(NULL, 0));
    ck_assert_int_eq(EINVAL, errno);
    
    jsonpath path;
    char *expression = "$";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 1);

    errno = 0;
    ck_assert_null(path_get_step(&path, 1));
    ck_assert_int_eq(EINVAL, errno);

    jsonpath_free(&path);
}
END_TEST

START_TEST (bad_step_input)
{
    errno = 0;
    ck_assert_int_eq(-1, step_get_kind(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(-1, step_get_test_kind(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(-1, type_test_step_get_type(NULL));
    ck_assert_int_eq(EINVAL, errno);
    
    errno = 0;
    ck_assert_false(step_has_predicate(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(step_get_predicate(NULL));
    ck_assert_int_eq(EINVAL, errno);
    
    jsonpath path;
    char *expression = "$.foo.array()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);

    errno = 0;
    ck_assert_int_eq(-1, type_test_step_get_type(path_get_step(&path, 1)));
    ck_assert_int_eq(EINVAL, errno);

    step *step2 = path_get_step(&path, 2);
    errno = 0;
    ck_assert_int_eq(0, name_test_step_get_length(step2));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(name_test_step_get_name(step2));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(step_get_predicate(step2));
    ck_assert_int_eq(EINVAL, errno);

    jsonpath_free(&path);
}
END_TEST

START_TEST (bad_predicate_input)
{
    errno = 0;
    ck_assert_int_eq(-1, predicate_get_kind(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(0, subscript_predicate_get_index(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(0, slice_predicate_get_from(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(0, slice_predicate_get_to(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(0, slice_predicate_get_step(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(join_predicate_get_left(NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(join_predicate_get_right(NULL));
    ck_assert_int_eq(EINVAL, errno);

    jsonpath path;
    char *expression = "$.foo[42].bar[*]";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);    
    assert_parser_result(expression, result, &path, ABSOLUTE_PATH, 3);

    predicate *subscript = step_get_predicate(path_get_step(&path, 1));
    errno = 0;
    ck_assert_int_eq(0, slice_predicate_get_to(subscript));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(0, slice_predicate_get_from(subscript));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_int_eq(0, slice_predicate_get_step(subscript));
    ck_assert_int_eq(EINVAL, errno);

    predicate *wildcard = step_get_predicate(path_get_step(&path, 2));
    errno = 0;
    ck_assert_int_eq(0, subscript_predicate_get_index(wildcard));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(join_predicate_get_left(wildcard));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(join_predicate_get_right(wildcard));
    ck_assert_int_eq(EINVAL, errno);

    jsonpath_free(&path);
}
END_TEST

Suite *jsonpath_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_expression);
    tcase_add_test(bad_input_case, zero_length);
    tcase_add_test(bad_input_case, null_path);
    tcase_add_test(bad_input_case, missing_step_test);
    tcase_add_test(bad_input_case, missing_recursive_step_test);
    tcase_add_test(bad_input_case, missing_dot);
    tcase_add_test(bad_input_case, relative_path_begins_with_dot);
    tcase_add_test(bad_input_case, quoted_empty_step);
    tcase_add_test(bad_input_case, bogus_type_test_name);
    tcase_add_test(bad_input_case, bogus_type_test_name_oblong);
    tcase_add_test(bad_input_case, bogus_type_test_name_alloy);
    tcase_add_test(bad_input_case, bogus_type_test_name_strong);
    tcase_add_test(bad_input_case, bogus_type_test_name_numbered);
    tcase_add_test(bad_input_case, bogus_type_test_name_booger);
    tcase_add_test(bad_input_case, bogus_type_test_name_narl);
    tcase_add_test(bad_input_case, empty_type_test_name);
    tcase_add_test(bad_input_case, empty_predicate);
    tcase_add_test(bad_input_case, whitespace_predicate);
    tcase_add_test(bad_input_case, extra_junk_in_predicate);
    tcase_add_test(bad_input_case, bogus_predicate);
    tcase_add_test(bad_input_case, wildcard_with_trailing_junk);
    tcase_add_test(bad_input_case, type_test_with_trailing_junk);

    TCase *basic_case = tcase_create("basic");
    tcase_add_test(basic_case, dollar_only);
    tcase_add_test(basic_case, absolute_single_step);
    tcase_add_test(basic_case, absolute_recursive_step);
    tcase_add_test(basic_case, absolute_multi_step);
    tcase_add_test(basic_case, quoted_multi_step);
    tcase_add_test(basic_case, relative_multi_step);
    tcase_add_test(basic_case, whitespace);
    tcase_add_test(basic_case, wildcard);
    tcase_add_test(basic_case, recursive_wildcard);

    TCase *node_type_case = tcase_create("node type test");
    tcase_add_test(node_type_case, type_test_missing_closing_paren);
    tcase_add_test(node_type_case, recursive_type_test);
    tcase_add_test(node_type_case, object_type_test);
    tcase_add_test(node_type_case, array_type_test);
    tcase_add_test(node_type_case, string_type_test);
    tcase_add_test(node_type_case, number_type_test);
    tcase_add_test(node_type_case, boolean_type_test);
    tcase_add_test(node_type_case, null_type_test);

    TCase *predicate_case = tcase_create("predicate");
    tcase_add_test(predicate_case, wildcard_predicate);
    tcase_add_test(predicate_case, wildcard_predicate_with_whitespace);
    tcase_add_test(predicate_case, subscript_predicate);
    tcase_add_test(predicate_case, subscript_predicate_with_whitespace);
    tcase_add_test(predicate_case, negative_subscript_predicate);

    TCase *api_case = tcase_create("api");
    tcase_add_test(api_case, bad_path_input);
    tcase_add_test(api_case, bad_step_input);
    tcase_add_test(api_case, bad_predicate_input);

    Suite *jsonpath_suite = suite_create("JSONPath");
    suite_add_tcase(jsonpath_suite, bad_input_case);
    suite_add_tcase(jsonpath_suite, basic_case);
    suite_add_tcase(jsonpath_suite, node_type_case);
    suite_add_tcase(jsonpath_suite, predicate_case);
    suite_add_tcase(jsonpath_suite, api_case);

    return jsonpath_suite;
}
