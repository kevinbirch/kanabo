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

#include "test.h"
#include "jsonpath.h"

#define assert_parser_result(EXPRESSION, RESULT, PATH, EXPECTED_KIND, EXPECTED_LENGTH) \
    char *_assert_message = make_status_message(&PATH);                 \
    assert_not_null(_assert_message);                                   \
    if(JSONPATH_SUCCESS != RESULT)                                      \
    {                                                                   \
        log_error("parser test", "failed for the expression: '%s', received: '%s'", EXPRESSION, _assert_message); \
    }                                                                   \
    free(_assert_message);                                              \
    assert_int_eq(JSONPATH_SUCCESS, RESULT);                            \
    assert_int_ne(0, PATH.result.position);                             \
    assert_int_eq(EXPECTED_KIND, path_get_kind(&PATH));                 \
    assert_not_null(PATH.steps);                                        \
    assert_int_eq(EXPECTED_LENGTH, path_get_length(&PATH))

#define assert_step(PATH, INDEX, EXPECTED_STEP_KIND, EXPECTED_TEST_KIND) \
    assert_int_eq(EXPECTED_STEP_KIND, step_get_kind(path_get_step(&PATH, INDEX))); \
    assert_int_eq(EXPECTED_TEST_KIND, step_get_test_kind(path_get_step(&PATH, INDEX)))

#define assert_name(CURRENT, NAME, LENGTH)                              \
    assert_int_eq(LENGTH, name_test_step_get_length(CURRENT));          \
    assert_buf_eq(NAME, LENGTH, name_test_step_get_name(CURRENT), name_test_step_get_length(CURRENT))

#define assert_no_predicates(PATH, INDEX)                           \
    assert_false(step_has_predicate(path_get_step(&PATH, INDEX)));  \
    assert_null(PATH.steps[INDEX]->predicate)

#define assert_root_step(PATH)                  \
    assert_step(PATH, 0, ROOT, NAME_TEST);      \
    assert_no_predicates(PATH, 0)

#define assert_name_step(PATH,INDEX, NAME, EXPECTED_STEP_KIND)          \
    assert_step(PATH, INDEX, EXPECTED_STEP_KIND, NAME_TEST);            \
    assert_name(path_get_step(&PATH, INDEX), (uint8_t *)NAME, strlen(NAME))
#define assert_single_name_step(PATH, INDEX, NAME) assert_name_step(PATH, INDEX, NAME, SINGLE)
#define assert_recursive_name_step(PATH, INDEX, NAME) assert_name_step(PATH, INDEX, NAME, RECURSIVE)

#define assert_wildcard_step(PATH, INDEX, EXPECTED_STEP_KIND)       \
    assert_step(PATH, INDEX, EXPECTED_STEP_KIND, WILDCARD_TEST);    \
    assert_name(path_get_step(&PATH, INDEX), (uint8_t *)"*", 1)
#define assert_single_wildcard_step(PATH, INDEX) assert_wildcard_step(PATH, INDEX, SINGLE)
#define assert_recursive_wildcard_step(PATH, INDEX) assert_wildcard_step(PATH, INDEX, RECURSIVE)

#define assert_type_step(PATH, INDEX, EXPECTED_TYPE_KIND, EXPECTED_STEP_KIND) \
    assert_step(PATH, INDEX, EXPECTED_STEP_KIND, TYPE_TEST);            \
    assert_int_eq(EXPECTED_TYPE_KIND, type_test_step_get_type(path_get_step(&PATH, INDEX)))
#define assert_single_type_step(PATH, INDEX, EXPECTED_TYPE_KIND)    \
    assert_type_step(PATH, INDEX, EXPECTED_TYPE_KIND, SINGLE)
#define assert_recursive_type_step(PATH, INDEX, EXPECTED_TYPE_KIND) \
    assert_type_step(PATH, INDEX, EXPECTED_TYPE_KIND, RECURSIVE)

#define assert_predicate(PATH, PATH_INDEX, EXPECTED_PREDICATE_KIND)     \
    assert_true(step_has_predicate(path_get_step(&PATH, PATH_INDEX)));  \
    assert_not_null(PATH.steps[PATH_INDEX]->predicate);                 \
    assert_not_null(step_get_predicate(path_get_step(&PATH, PATH_INDEX))); \
    assert_int_eq(EXPECTED_PREDICATE_KIND, predicate_get_kind(step_get_predicate(path_get_step(&PATH, PATH_INDEX))))
#define assert_wildcard_predicate(PATH, PATH_INDEX) assert_predicate(PATH, PATH_INDEX, WILDCARD)
#define assert_subscript_predicate(PATH, PATH_INDEX, INDEX_VALUE)       \
    assert_predicate(PATH, PATH_INDEX, SUBSCRIPT);                      \
    assert_int_eq(INDEX_VALUE, subscript_predicate_get_index(step_get_predicate(path_get_step(&PATH, PATH_INDEX))))
#define assert_slice_predicate(PATH, PATH_INDEX, FROM_VALUE, TO_VALUE, STEP_VALUE) \
    assert_predicate(path, PATH_INDEX, SLICE);                          \
    assert_int_eq(FROM_VALUE, slice_predicate_get_from(step_get_predicate(path_get_step(&PATH, PATH_INDEX)))); \
    assert_int_eq(TO_VALUE,   slice_predicate_get_to(step_get_predicate(path_get_step(&PATH, PATH_INDEX)))); \
    assert_int_eq(STEP_VALUE, slice_predicate_get_step(step_get_predicate(path_get_step(&PATH, PATH_INDEX))))

START_TEST (null_expression)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath(NULL, 50, &path);
    
    assert_int_eq(ERR_NULL_EXPRESSION, result);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    assert_int_eq(0, path.result.position);
}
END_TEST

START_TEST (zero_length)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"", 0, &path);
    
    assert_int_eq(ERR_ZERO_LENGTH, result);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    assert_int_eq(0, path.result.position);
}
END_TEST

START_TEST (null_path)
{
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"", 5, NULL);
    
    assert_int_eq(ERR_NULL_OUTPUT_PATH, result);
}
END_TEST

START_TEST (missing_step_test)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$.", 2, &path);
    
    assert_int_eq(ERR_PREMATURE_END_OF_INPUT, result);
    assert_int_eq(2, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (missing_recursive_step_test)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$..", 3, &path);
    
    assert_int_eq(ERR_PREMATURE_END_OF_INPUT, result);
    assert_int_eq(3, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (missing_dot)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)"$x", 2, &path);
    
    assert_int_eq(ERR_UNEXPECTED_VALUE, result);
    assert_int_eq(1, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (relative_path_begins_with_dot)
{
    jsonpath path;
    jsonpath_status_code result = parse_jsonpath((uint8_t *)".x", 2, &path);
    
    assert_int_eq(ERR_EXPECTED_NAME_CHAR, result);
    assert_int_eq(0, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NAME_CHAR, result);
    assert_int_eq(7, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EMPTY_PREDICATE, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXTRA_JUNK_AFTER_PREDICATE, result);
    assert_int_eq(9, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EMPTY_PREDICATE, result);
    assert_int_eq(9, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    jsonpath_free(&path);
}
END_TEST

START_TEST (bogus_predicate)
{
    jsonpath path;
    char *expression = "$.foo[asdf].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_int_eq(ERR_UNSUPPORTED_PRED_TYPE, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_int_eq(ERR_EXPECTED_NODE_TYPE_TEST, result);
    assert_int_eq(6, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);
    jsonpath_free(&path);
}
END_TEST

START_TEST (dollar_only)
{
    jsonpath path;
    char *expression = "$";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, 1, &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 1);
    assert_root_step(path);

    jsonpath_free(&path);
}
END_TEST

START_TEST (absolute_single_step)
{
    jsonpath path;
    char *expression = "$.foo";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, 5, &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 2);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_no_predicates(path, 0);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (absolute_recursive_step)
{
    jsonpath path;
    char *expression = "$..foo";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, 6, &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 2);
    assert_root_step(path);
    assert_recursive_name_step(path, 1, "foo");
    assert_no_predicates(path, 0);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (absolute_multi_step)
{
    jsonpath path;
    char *expression = "$.foo.baz..yobble.thingum";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 5);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_name_step(path, 2, "baz");
    assert_recursive_name_step(path, 3, "yobble");
    assert_single_name_step(path, 4, "thingum");
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);
    assert_no_predicates(path, 3);
    assert_no_predicates(path, 4);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (relative_multi_step)
{
    jsonpath path;
    char *expression = "foo.bar..baz";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, RELATIVE_PATH, 3);
    assert_single_name_step(path, 0, "foo");
    assert_single_name_step(path, 1, "bar");
    assert_recursive_name_step(path, 2, "baz");
    assert_no_predicates(path, 0);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (quoted_multi_step)
{
    jsonpath path;
    char *expression = "$.foo.'happy fun ball'.bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 4);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_name_step(path, 2, "happy fun ball");
    assert_single_name_step(path, 3, "bar");
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);
    assert_no_predicates(path, 3);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (wildcard)
{
    jsonpath path;
    char *expression = "$.foo.*";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_wildcard_step(path, 2);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (recursive_wildcard)
{
    jsonpath path;
    char *expression = "$.foo..*";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_recursive_wildcard_step(path, 2);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_with_trailing_junk)
{
    jsonpath path;
    char *expression = "$.foo.* [0]";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_int_eq(ERR_EXTRA_JUNK_AFTER_WILDCARD, result);
    assert_int_eq(7, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 4);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_recursive_name_step(path, 2, "happy fun ball");
    assert_single_type_step(path, 3, STRING_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);
    assert_no_predicates(path, 3);

    jsonpath_free(&path);    
}
END_TEST

START_TEST (type_test_missing_closing_paren)
{
    jsonpath path;
    char *expression = "$.foo.null(";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_name_step(path, 2, "null(");
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (type_test_with_trailing_junk)
{
    jsonpath path;
    char *expression = "$.foo.array()[0]";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_int_eq(ERR_EXTRA_JUNK_AFTER_TYPE_TEST, result);
    assert_int_eq(13, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

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
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_recursive_type_step(path, 2, STRING_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (object_type_test)
{
    jsonpath path;
    char *expression = "$.foo.object()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, OBJECT_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (array_type_test)
{
    jsonpath path;
    char *expression = "$.foo.array()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, ARRAY_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (string_type_test)
{
    jsonpath path;
    char *expression = "$.foo.string()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, STRING_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (number_type_test)
{
    jsonpath path;
    char *expression = "$.foo.number()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, NUMBER_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (boolean_type_test)
{
    jsonpath path;
    char *expression = "$.foo.boolean()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, BOOLEAN_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (null_type_test)
{
    jsonpath path;
    char *expression = "$.foo.null()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, NULL_TEST);
    assert_no_predicates(path, 1);
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_predicate)
{
    jsonpath path;
    char *expression = "$.store.book[*].author";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 4);
    assert_root_step(path);
    assert_single_name_step(path, 1, "store");
    assert_no_predicates(path, 1);
    assert_single_name_step(path, 2, "book");
    assert_wildcard_predicate(path, 2);
    assert_single_name_step(path, 3, "author");
    assert_no_predicates(path, 3);

    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_predicate_with_whitespace)
{
    jsonpath path;
    char *expression = "$.foo  [\t*\n]  .bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_wildcard_predicate(path, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (subscript_predicate)
{
    jsonpath path;
    char *expression = "$.foo[42].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_subscript_predicate(path, 1, 42);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (subscript_predicate_with_whitespace)
{
    jsonpath path;
    char *expression = "$.foo  [\t42\r]\n.bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_subscript_predicate(path, 1, 42);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (negative_subscript_predicate)
{
    jsonpath path;
    char *expression = "$.foo[ -3].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    // xxx - fixme! this should be ERR_EXPECTED_INTEGER instead!
    assert_int_eq(ERR_UNSUPPORTED_PRED_TYPE, result);
    assert_int_eq(7, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);

    jsonpath_free(&path);
}
END_TEST

START_TEST (slice_predicate_form1)
{
    jsonpath path;
    char *expression = "$.foo[:-3].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, INT_FAST32_MIN, -3, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (slice_predicate_form1_with_step)
{
    jsonpath path;
    char *expression = "$.foo[:-3:2].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, INT_FAST32_MIN, -3, 2);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (slice_predicate_form2)
{
    jsonpath path;
    char *expression = "$.foo[-3:].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, -3, INT_FAST32_MAX, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (slice_predicate_form2_with_step)
{
    jsonpath path;
    char *expression = "$.foo[-1::2].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, -1, INT_FAST32_MAX, 2);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (slice_predicate_form3)
{
    jsonpath path;
    char *expression = "$.foo[3:5].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, 3, 5, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (slice_predicate_form3_with_step)
{
    jsonpath path;
    char *expression = "$.foo[1:4:2].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, 1, 4, 2);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (slice_predicate_with_whitespace)
{
    jsonpath path;
    char *expression = "$.foo  [\t1\t:\t5\r:\n3\t]\n.bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, 1, 5, 3);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicates(path, 2);

    jsonpath_free(&path);
}
END_TEST

START_TEST (negative_step_slice_predicate)
{
    jsonpath path;
    char *expression = "$.foo[1:3:-3].bar";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    
    // xxx - fixme! this should be ERR_EXPECTED_INTEGER instead!
    assert_int_eq(ERR_UNSUPPORTED_PRED_TYPE, result);
    assert_int_eq(10, path.result.position);
    char *message = make_status_message(&path);
    assert_not_null(message);

    log_info("parser test", "received expected failure message: '%s'", message);

    free(message);

    jsonpath_free(&path);
}
END_TEST

START_TEST (bad_path_input)
{
    errno = 0;
    assert_int_eq(-1, path_get_kind(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(0, path_get_length(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(path_get_step(NULL, 0));
    assert_int_eq(EINVAL, errno);
    
    jsonpath path;
    char *expression = "$";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 1);

    errno = 0;
    assert_null(path_get_step(&path, 1));
    assert_int_eq(EINVAL, errno);

    jsonpath_free(&path);
}
END_TEST

START_TEST (bad_step_input)
{
    errno = 0;
    assert_int_eq(-1, step_get_kind(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(-1, step_get_test_kind(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(-1, type_test_step_get_type(NULL));
    assert_int_eq(EINVAL, errno);
    
    errno = 0;
    assert_false(step_has_predicate(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(step_get_predicate(NULL));
    assert_int_eq(EINVAL, errno);
    
    jsonpath path;
    char *expression = "$.foo.array()";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);

    errno = 0;
    assert_int_eq(-1, type_test_step_get_type(path_get_step(&path, 1)));
    assert_int_eq(EINVAL, errno);

    step *step2 = path_get_step(&path, 2);
    errno = 0;
    assert_int_eq(0, name_test_step_get_length(step2));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(name_test_step_get_name(step2));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(step_get_predicate(step2));
    assert_int_eq(EINVAL, errno);

    jsonpath_free(&path);
}
END_TEST

START_TEST (bad_predicate_input)
{
    errno = 0;
    assert_int_eq(-1, predicate_get_kind(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(0, subscript_predicate_get_index(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(0, slice_predicate_get_from(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(0, slice_predicate_get_to(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(0, slice_predicate_get_step(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(join_predicate_get_left(NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(join_predicate_get_right(NULL));
    assert_int_eq(EINVAL, errno);

    jsonpath path;
    char *expression = "$.foo[42].bar[*]";
    jsonpath_status_code result = parse_jsonpath((uint8_t *)expression, strlen(expression), &path);    
    assert_parser_result(expression, result, path, ABSOLUTE_PATH, 3);

    predicate *subscript = step_get_predicate(path_get_step(&path, 1));
    errno = 0;
    assert_int_eq(0, slice_predicate_get_to(subscript));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(0, slice_predicate_get_from(subscript));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_int_eq(0, slice_predicate_get_step(subscript));
    assert_int_eq(EINVAL, errno);

    predicate *wildcard = step_get_predicate(path_get_step(&path, 2));
    errno = 0;
    assert_int_eq(0, subscript_predicate_get_index(wildcard));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(join_predicate_get_left(wildcard));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(join_predicate_get_right(wildcard));
    assert_int_eq(EINVAL, errno);

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
    tcase_add_test(predicate_case, slice_predicate_form1);
    tcase_add_test(predicate_case, slice_predicate_form1_with_step);
    tcase_add_test(predicate_case, slice_predicate_form2);
    tcase_add_test(predicate_case, slice_predicate_form2_with_step);
    tcase_add_test(predicate_case, slice_predicate_form3);
    tcase_add_test(predicate_case, slice_predicate_form3_with_step);
    tcase_add_test(predicate_case, slice_predicate_with_whitespace);
    tcase_add_test(predicate_case, negative_step_slice_predicate);

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
