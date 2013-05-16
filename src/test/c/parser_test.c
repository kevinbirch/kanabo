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

#define assert_path_length(PATH, EXPECTED) assert_uint_eq((EXPECTED), path_length((PATH)))
#define assert_path_kind(PATH, EXPECTED) assert_int_eq((EXPECTED), path_kind((PATH)))

#define assert_parser_success(EXPRESSION, CONTEXT, PATH, EXPECTED_KIND, EXPECTED_LENGTH) \
    do                                                                  \
    {                                                                   \
        assert_not_null((CONTEXT));                                     \
        if(JSONPATH_SUCCESS != parser_status((CONTEXT)))                \
        {                                                               \
            char *_assert_message = parser_status_message((CONTEXT));   \
            assert_not_null(_assert_message);                           \
            log_error("parser test", "for the expression: '%s', received: '%s'", (EXPRESSION), _assert_message); \
            free(_assert_message);                                      \
        }                                                               \
        assert_int_eq(JSONPATH_SUCCESS, parser_status((CONTEXT)));      \
        assert_uint_ne(0, (CONTEXT)->cursor);                           \
        assert_path_kind((PATH), (EXPECTED_KIND));                      \
        assert_not_null((PATH)->steps);                                 \
        assert_null((CONTEXT)->steps);                                  \
        assert_path_length((PATH), (EXPECTED_LENGTH));                  \
    } while(0)

#define assert_parser_failure(EXPRESSION, CONTEXT, PATH, EXPECTED_RESULT, EXPECTED_POSITION) \
    do                                                                  \
    {                                                                   \
        assert_null((PATH));                                            \
        assert_not_null((CONTEXT));                                     \
        assert_int_eq((EXPECTED_RESULT), parser_status((CONTEXT)));     \
        char *_assert_message = parser_status_message((CONTEXT));       \
        assert_not_null(_assert_message);                               \
        log_debug("parser test", "for expression: '%s', received expected failure message: '%s'", (EXPRESSION), _assert_message); \
        free(_assert_message);                                          \
        assert_uint_eq((EXPECTED_POSITION), (CONTEXT)->cursor);         \
    } while(0)

#define assert_step_kind(STEP, EXPECTED_KIND) assert_int_eq(EXPECTED_KIND, step_kind(STEP))
#define assert_test_kind(STEP, EXPECTED_KIND) assert_int_eq(EXPECTED_KIND, step_test_kind(STEP))

#define assert_step(PATH, INDEX, EXPECTED_STEP_KIND, EXPECTED_TEST_KIND) \
    assert_step_kind(path_get((PATH), INDEX), EXPECTED_STEP_KIND);  \
    assert_test_kind(path_get((PATH), INDEX), EXPECTED_TEST_KIND)

#define assert_name_length(STEP, NAME) assert_uint_eq(strlen(NAME), name_test_step_length(STEP))
#define assert_name(STEP, NAME)                                         \
    assert_name_length(STEP, NAME);                                     \
    assert_buf_eq(NAME, strlen(NAME), name_test_step_name(STEP), name_test_step_length(STEP))

#define assert_no_predicate(PATH, INDEX)                             \
    assert_false(step_has_predicate(path_get((PATH), INDEX)));  \
    assert_null((PATH)->steps[INDEX]->predicate)

#define assert_root_step(PATH)                  \
    assert_step((PATH), 0, ROOT, NAME_TEST);    \
    assert_no_predicate((PATH), 0)

#define assert_name_step(PATH,INDEX, NAME, EXPECTED_STEP_KIND)      \
    assert_step((PATH), INDEX, EXPECTED_STEP_KIND, NAME_TEST);      \
    assert_name(path_get((PATH), INDEX), NAME)
#define assert_single_name_step(PATH, INDEX, NAME) assert_name_step((PATH), INDEX, NAME, SINGLE)
#define assert_recursive_name_step(PATH, INDEX, NAME) assert_name_step((PATH), INDEX, NAME, RECURSIVE)

#define assert_wildcard_step(PATH, INDEX, EXPECTED_STEP_KIND) assert_step((PATH), INDEX, EXPECTED_STEP_KIND, WILDCARD_TEST)
#define assert_single_wildcard_step(PATH, INDEX) assert_wildcard_step((PATH), INDEX, SINGLE)
#define assert_recursive_wildcard_step(PATH, INDEX) assert_wildcard_step((PATH), INDEX, RECURSIVE)

#define assert_type_kind(STEP, EXPECTED) assert_int_eq(EXPECTED, type_test_step_kind(STEP))

#define assert_type_step(PATH, INDEX, EXPECTED_TYPE_KIND, EXPECTED_STEP_KIND) \
    assert_step((PATH), INDEX, EXPECTED_STEP_KIND, TYPE_TEST);            \
    assert_type_kind(path_get((PATH), INDEX), EXPECTED_TYPE_KIND)

#define assert_single_type_step(PATH, INDEX, EXPECTED_TYPE_KIND)    \
    assert_type_step((PATH), INDEX, EXPECTED_TYPE_KIND, SINGLE)
#define assert_recursive_type_step(PATH, INDEX, EXPECTED_TYPE_KIND) \
    assert_type_step((PATH), INDEX, EXPECTED_TYPE_KIND, RECURSIVE)

#define assert_predicate_kind(PREDICATE, EXPECTED) assert_int_eq(EXPECTED, predicate_kind(PREDICATE))

#define assert_predicate(PATH, PATH_INDEX, EXPECTED_PREDICATE_KIND)     \
    assert_true(step_has_predicate(path_get((PATH), PATH_INDEX))); \
    assert_not_null((PATH)->steps[PATH_INDEX]->predicate);              \
    assert_not_null(step_predicate(path_get((PATH), PATH_INDEX))); \
    assert_predicate_kind(step_predicate(path_get((PATH), PATH_INDEX)), EXPECTED_PREDICATE_KIND)

#define assert_wildcard_predicate(PATH, PATH_INDEX) assert_predicate((PATH), PATH_INDEX, WILDCARD)

#define assert_subscript_index(PREDICATE, VALUE) assert_uint_eq(VALUE, subscript_predicate_index(PREDICATE))
#define assert_subscript_predicate(PATH, PATH_INDEX, INDEX_VALUE)       \
    assert_predicate((PATH), PATH_INDEX, SUBSCRIPT);                    \
    assert_subscript_index(step_predicate(path_get((PATH), PATH_INDEX)), INDEX_VALUE);

#define assert_slice_from(PREDICATE, VALUE) assert_int_eq(VALUE, slice_predicate_from(PREDICATE))
#define assert_slice_to(PREDICATE, VALUE) assert_int_eq(VALUE, slice_predicate_to(PREDICATE))
#define assert_slice_step(PREDICATE, VALUE) assert_int_eq(VALUE, slice_predicate_step(PREDICATE))
#define assert_slice_predicate(PATH, PATH_INDEX, FROM_VALUE, TO_VALUE, STEP_VALUE) \
    assert_predicate(path, PATH_INDEX, SLICE);                          \
    assert_slice_from(step_predicate(path_get((PATH), PATH_INDEX)), FROM_VALUE); \
    assert_slice_to(step_predicate(path_get((PATH), PATH_INDEX)), TO_VALUE); \
    assert_slice_step(step_predicate(path_get(((PATH)), PATH_INDEX)), STEP_VALUE)

static bool count(step *each, void *context);
static bool fail_count(step *each, void *context);

START_TEST (null_expression)
{
    char *expression = NULL;
    parser_context *context = make_parser((uint8_t *)expression, 50);
    assert_not_null(context);
    assert_errno(EINVAL);
    
    assert_parser_failure(expression, context, NULL, ERR_NULL_EXPRESSION, 0);
    parser_free(context);
}
END_TEST

START_TEST (zero_length)
{
    char *expression = "";
    parser_context *context = make_parser((uint8_t *)expression, 0);
    assert_not_null(context);
    assert_errno(EINVAL);

    assert_parser_failure(expression, context, NULL, ERR_ZERO_LENGTH, 0);
    parser_free(context);
}
END_TEST

START_TEST (bogus_context)
{
    reset_errno();
    assert_null(parse(NULL));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (bogus_context_path)
{
    uint8_t bogus_input = 64;
    
    parser_context context;
    context.path = NULL;
    context.input = &bogus_input;
    context.length = 5;
    
    reset_errno();
    assert_null(parse(&context));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (bogus_context_input)
{
    jsonpath bogus_path;
    
    parser_context context;
    context.path = &bogus_path;
    context.input = NULL;
    context.length = 5;

    reset_errno();
    assert_null(parse(&context));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (bogus_context_length)
{
    jsonpath bogus_path;
    uint8_t bogus_input = 64;
    
    parser_context context;
    context.path = &bogus_path;
    context.input = &bogus_input;
    context.length = 0;

    reset_errno();
    assert_null(parse(&context));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (missing_step_test)
{
    char *expression = "$.";
    parser_context *context = make_parser((uint8_t *)expression, 2);
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_PREMATURE_END_OF_INPUT, 2);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (missing_recursive_step_test)
{
    char *expression = "$..";
    parser_context *context = make_parser((uint8_t *)expression, 3);
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_PREMATURE_END_OF_INPUT, 3);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (missing_dot)
{
    char *expression = "$x";
    parser_context *context = make_parser((uint8_t *)expression, 2);
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_UNEXPECTED_VALUE, 1);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (relative_path_begins_with_dot)
{
    char *expression = ".x";
    parser_context *context = make_parser((uint8_t *)expression, 2);
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NAME_CHAR, 0);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (quoted_empty_step)
{
    char *expression = "$.foo.''.bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NAME_CHAR, 7);
    
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (empty_predicate)
{
    char *expression = "$.foo[].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EMPTY_PREDICATE, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (extra_junk_in_predicate)
{
    char *expression = "$.foo[ * quux].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXTRA_JUNK_AFTER_PREDICATE, 9);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (whitespace_predicate)
{
    char *expression = "$.foo[ \t ].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EMPTY_PREDICATE, 9);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_predicate)
{
    char *expression = "$.foo[asdf].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_UNSUPPORTED_PRED_TYPE, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_type_test_name)
{
    char *expression = "$.foo.monkey()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_type_test_name_oblong)
{
    char *expression = "$.foo.oblong()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_type_test_name_alloy)
{
    char *expression = "$.foo.alloy()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_type_test_name_strong)
{
    char *expression = "$.foo.strong()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_type_test_name_numbered)
{
    char *expression = "$.foo.numbered()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_type_test_name_booger)
{
    char *expression = "$.foo.booger()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (bogus_type_test_name_narl)
{
    char *expression = "$.foo.narl()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (empty_type_test_name)
{
    char *expression = "$.foo.()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_failure(expression, context, path, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    parser_free(context);
    path_free(path);
}
END_TEST

START_TEST (dollar_only)
{
    char *expression = "$";
    parser_context *context = make_parser((uint8_t *)expression, 1);
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 1);
    assert_root_step(path);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (absolute_single_step)
{
    char *expression = "$.foo";
    parser_context *context = make_parser((uint8_t *)expression, 5);
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 2);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_no_predicate(path, 0);

    path_free(path);    
    parser_free(context);
}
END_TEST

START_TEST (absolute_recursive_step)
{
    char *expression = "$..foo";
    parser_context *context = make_parser((uint8_t *)expression, 6);
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 2);
    assert_root_step(path);
    assert_recursive_name_step(path, 1, "foo");
    assert_no_predicate(path, 0);

    path_free(path);    
    parser_free(context);
}
END_TEST

START_TEST (absolute_multi_step)
{
    char *expression = "$.foo.baz..yobble.thingum";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 5);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_name_step(path, 2, "baz");
    assert_recursive_name_step(path, 3, "yobble");
    assert_single_name_step(path, 4, "thingum");
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);
    assert_no_predicate(path, 3);
    assert_no_predicate(path, 4);

    path_free(path);    
    parser_free(context);
}
END_TEST

START_TEST (relative_multi_step)
{
    char *expression = "foo.bar..baz";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, RELATIVE_PATH, 3);
    assert_single_name_step(path, 0, "foo");
    assert_single_name_step(path, 1, "bar");
    assert_recursive_name_step(path, 2, "baz");
    assert_no_predicate(path, 0);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);    
    parser_free(context);
}
END_TEST

START_TEST (quoted_multi_step)
{
    char *expression = "$.foo.'happy fun ball'.bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 4);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_name_step(path, 2, "happy fun ball");
    assert_single_name_step(path, 3, "bar");
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);
    assert_no_predicate(path, 3);

    path_free(path);    
    parser_free(context);
}
END_TEST

START_TEST (wildcard)
{
    char *expression = "$.foo.*";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_wildcard_step(path, 2);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (recursive_wildcard)
{
    char *expression = "$.foo..*";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_recursive_wildcard_step(path, 2);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (wildcard_with_subscript_predicate)
{
    char *expression = "$.foo.* [0]";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);

    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_no_predicate(path, 1);
    assert_single_wildcard_step(path, 2);
    assert_subscript_predicate(path, 2, 0);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (whitespace)
{
    char *expression = "  $ \r\n. foo \n.\n. \t'happy fun ball' . \t string()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 4);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_recursive_name_step(path, 2, "happy fun ball");
    assert_single_type_step(path, 3, STRING_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);
    assert_no_predicate(path, 3);

    path_free(path);    
    parser_free(context);
}
END_TEST

START_TEST (type_test_missing_closing_paren)
{
    char *expression = "$.foo.null(";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_name_step(path, 2, "null(");
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (recursive_type_test)
{
    char *expression = "$.foo..string()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_recursive_type_step(path, 2, STRING_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (object_type_test)
{
    char *expression = "$.foo.object()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, OBJECT_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (array_type_test)
{
    char *expression = "$.foo.array()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, ARRAY_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (string_type_test)
{
    char *expression = "$.foo.string()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, STRING_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (number_type_test)
{
    char *expression = "$.foo.number()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, NUMBER_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (boolean_type_test)
{
    char *expression = "$.foo.boolean()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, BOOLEAN_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (null_type_test)
{
    char *expression = "$.foo.null()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_single_type_step(path, 2, NULL_TEST);
    assert_no_predicate(path, 1);
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (wildcard_predicate)
{
    char *expression = "$.store.book[*].author";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 4);
    assert_root_step(path);
    assert_single_name_step(path, 1, "store");
    assert_no_predicate(path, 1);
    assert_single_name_step(path, 2, "book");
    assert_wildcard_predicate(path, 2);
    assert_single_name_step(path, 3, "author");
    assert_no_predicate(path, 3);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (wildcard_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t*\n]  .bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_wildcard_predicate(path, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.foo[42].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_subscript_predicate(path, 1, 42);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (subscript_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t42\r]\n.bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_subscript_predicate(path, 1, 42);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (type_test_with_subscript_predicate)
{
    char *expression = "$.foo.array()[0]";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_no_predicate(path, 1);
    assert_single_type_step(path, 2, ARRAY_TEST);
    assert_subscript_predicate(path, 2, 0);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (negative_subscript_predicate)
{
    char *expression = "$.foo[ -3].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    // xxx - fixme! this should be ERR_EXPECTED_INTEGER instead!
    assert_parser_failure(expression, context, path, ERR_UNSUPPORTED_PRED_TYPE, 7);
    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (slice_predicate_form1)
{
    char *expression = "$.foo[:-3].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, INT_FAST32_MIN, -3, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (slice_predicate_form1_with_step)
{
    char *expression = "$.foo[:-3:2].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, INT_FAST32_MIN, -3, 2);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (slice_predicate_form2)
{
    char *expression = "$.foo[-3:].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, -3, INT_FAST32_MAX, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (slice_predicate_form2_with_step)
{
    char *expression = "$.foo[-1::2].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, -1, INT_FAST32_MAX, 2);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (slice_predicate_form3)
{
    char *expression = "$.foo[3:5].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, 3, 5, 1);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (slice_predicate_form3_with_step)
{
    char *expression = "$.foo[1:4:2].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, 1, 4, 2);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (slice_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t1\t:\t5\r:\n3\t]\n.bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, 1, 5, 3);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (negative_step_slice_predicate)
{
    char *expression = "$.foo[1:3:-3].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);
    assert_root_step(path);
    assert_single_name_step(path, 1, "foo");
    assert_slice_predicate(path, 1, 1, 3, -3);
    assert_single_name_step(path, 2, "bar");
    assert_no_predicate(path, 2);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (zero_step_slice_predicate)
{
    char *expression = "$.foo[::0].bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    // xxx - fix me! this should be ERR_STEP_CANNOT_BE_ZERO instead
    // xxx - fix me! this should be position 8 instead, need a non-zero signed int parser
    assert_parser_failure(expression, context, path, ERR_UNSUPPORTED_PRED_TYPE, 9);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (iteration)
{
    char *expression = "$.foo.bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);

    unsigned long counter = 0;
    assert_true(path_iterate(path, count, &counter));
    assert_uint_eq(3, counter);

    path_free(path);
    parser_free(context);
}
END_TEST

static bool count(step *each, void *context)
{
#pragma unused(each)
    unsigned long *counter = (unsigned long *)context;
    (*counter)++;
    return true;
}

START_TEST (fail_iteration)
{
    char *expression = "$.foo.bar";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);

    unsigned long counter = 0;
    assert_false(path_iterate(path, fail_count, &counter));
    assert_uint_eq(1, counter);

    path_free(path);
    parser_free(context);
}
END_TEST

static bool fail_count(step *each, void *context)
{
#pragma unused(each)
    unsigned long *counter = (unsigned long *)context;
    if(0 == *counter)
    {
        (*counter)++;
        return true;
    }
    return false;
}

START_TEST (bad_path_input)
{
    reset_errno();
    assert_path_length(NULL, 0);
    assert_errno(EINVAL);
    reset_errno();
    assert_null(path_get(NULL, 0));
    assert_errno(EINVAL);
    
    reset_errno();
    char *expression = "$";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 1);

    reset_errno();
    assert_null(path_get(path, 1));
    assert_errno(EINVAL);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (bad_step_input)
{
    reset_errno();
    assert_false(step_has_predicate(NULL));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(step_predicate(NULL));
    assert_errno(EINVAL);
    
    reset_errno();
    char *expression = "$.foo.array()";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);

    step *step2 = path_get(path, 2);
    reset_errno();
    assert_uint_eq(0, name_test_step_length(step2));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(name_test_step_name(step2));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(step_predicate(step2));
    assert_errno(EINVAL);

    path_free(path);
    parser_free(context);
}
END_TEST

START_TEST (bad_predicate_input)
{
    reset_errno();
    assert_subscript_index(NULL, 0);
    assert_errno(EINVAL);

    reset_errno();
    assert_slice_from(NULL, 0);
    assert_errno(EINVAL);

    reset_errno();
    assert_slice_to(NULL, 0);
    assert_errno(EINVAL);

    reset_errno();
    assert_slice_step(NULL, 0);
    assert_errno(EINVAL);

    reset_errno();
    assert_null(join_predicate_left(NULL));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(join_predicate_right(NULL));
    assert_errno(EINVAL);

    reset_errno();
    char *expression = "$.foo[42].bar[*]";
    parser_context *context = make_parser((uint8_t *)expression, strlen(expression));    
    assert_not_null(context);
    assert_noerr();

    jsonpath *path = parse(context);
    assert_parser_success(expression, context, path, ABSOLUTE_PATH, 3);

    predicate *subscript = step_predicate(path_get(path, 1));
    reset_errno();
    assert_int_eq(0, slice_predicate_to(subscript));
    assert_errno(EINVAL);
    reset_errno();
    assert_int_eq(0, slice_predicate_from(subscript));
    assert_errno(EINVAL);
    reset_errno();
    assert_int_eq(0, slice_predicate_step(subscript));
    assert_errno(EINVAL);

    predicate *wildcard = step_predicate(path_get(path, 2));
    reset_errno();
    assert_uint_eq(0, subscript_predicate_index(wildcard));
    assert_errno(EINVAL);
    reset_errno();
    assert_null(join_predicate_left(wildcard));
    assert_errno(EINVAL);
    reset_errno();
    assert_null(join_predicate_right(wildcard));
    assert_errno(EINVAL);

    path_free(path);
    parser_free(context);
}
END_TEST

Suite *jsonpath_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_expression);
    tcase_add_test(bad_input_case, zero_length);
    tcase_add_test(bad_input_case, bogus_context);
    tcase_add_test(bad_input_case, bogus_context_input);
    tcase_add_test(bad_input_case, bogus_context_path);
    tcase_add_test(bad_input_case, bogus_context_length);
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
    tcase_add_test(basic_case, wildcard_with_subscript_predicate);

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
    tcase_add_test(predicate_case, type_test_with_subscript_predicate);
    tcase_add_test(predicate_case, negative_subscript_predicate);
    tcase_add_test(predicate_case, slice_predicate_form1);
    tcase_add_test(predicate_case, slice_predicate_form1_with_step);
    tcase_add_test(predicate_case, slice_predicate_form2);
    tcase_add_test(predicate_case, slice_predicate_form2_with_step);
    tcase_add_test(predicate_case, slice_predicate_form3);
    tcase_add_test(predicate_case, slice_predicate_form3_with_step);
    tcase_add_test(predicate_case, slice_predicate_with_whitespace);
    tcase_add_test(predicate_case, negative_step_slice_predicate);
    tcase_add_test(predicate_case, zero_step_slice_predicate);

    TCase *api_case = tcase_create("api");
    tcase_add_test(api_case, bad_path_input);
    tcase_add_test(api_case, bad_step_input);
    tcase_add_test(api_case, bad_predicate_input);
    tcase_add_test(api_case, iteration);
    tcase_add_test(api_case, fail_iteration);

    Suite *jsonpath_suite = suite_create("JSONPath");
    suite_add_tcase(jsonpath_suite, bad_input_case);
    suite_add_tcase(jsonpath_suite, basic_case);
    suite_add_tcase(jsonpath_suite, node_type_case);
    suite_add_tcase(jsonpath_suite, predicate_case);
    suite_add_tcase(jsonpath_suite, api_case);

    return jsonpath_suite;
}
