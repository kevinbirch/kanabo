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
#include "jsonpath/model.h"

#define assert_path_length(PATH, EXPECTED) assert_uint_eq((EXPECTED), path_length((PATH)))
#define assert_path_kind(PATH, EXPECTED) assert_int_eq((EXPECTED), path_kind((PATH)))

#define assert_parser_success(EXPRESSION, MAYBE, EXPECTED_KIND, EXPECTED_LENGTH) \
    do                                                                  \
    {                                                                   \
        if(PATH_ERROR == (MAYBE).tag)                                        \
        {                                                               \
            assert_not_null((MAYBE).error.message);                     \
            log_error("parser test", "for the expression: '%s', received: '%s'", (EXPRESSION), (MAYBE).error.message); \
        }                                                               \
        assert_int_eq(JSONPATH, (MAYBE).tag);                           \
        assert_path_kind((MAYBE).value, (EXPECTED_KIND));                \
        assert_not_null((MAYBE).value->steps);                           \
        assert_path_length((MAYBE).value, (EXPECTED_LENGTH));            \
    } while(0)

#define assert_parser_failure(EXPRESSION, MAYBE, EXPECTED_RESULT, EXPECTED_POSITION) \
    do                                                                  \
    {                                                                   \
        assert_int_eq(PATH_ERROR, (MAYBE).tag);                              \
        assert_int_eq((EXPECTED_RESULT), (MAYBE).error.code);           \
        assert_not_null((MAYBE).error.message);                         \
        log_debug("parser test", "for expression: '%s', received expected failure message: '%s'", (EXPRESSION), (MAYBE).error.message); \
        assert_uint_eq((EXPECTED_POSITION), (MAYBE).error.position);    \
    } while(0)

#define assert_step_kind(STEP, EXPECTED_KIND) assert_int_eq((EXPECTED_KIND), step_kind((STEP)))
#define assert_test_kind(STEP, EXPECTED_KIND) assert_int_eq((EXPECTED_KIND), step_test_kind((STEP)))

#define assert_step(PATH, INDEX, EXPECTED_STEP_KIND, EXPECTED_TEST_KIND) \
    assert_step_kind(path_get((PATH), (INDEX)), (EXPECTED_STEP_KIND));   \
    assert_test_kind(path_get((PATH), (INDEX)), (EXPECTED_TEST_KIND))

#define assert_name_length(STEP, NAME) assert_uint_eq(strlen((NAME)), name_test_step_length((STEP)))
#define assert_name(STEP, NAME)                                         \
    assert_name_length((STEP), (NAME));                                  \
    assert_buf_eq((NAME), strlen((NAME)), name_test_step_name((STEP)), name_test_step_length((STEP)))

#define assert_no_predicate(PATH, INDEX)                             \
    assert_false(step_has_predicate(path_get((PATH), (INDEX))));      \
    assert_null(path_get((PATH), (INDEX))->predicate)

#define assert_root_step(PATH)                  \
    assert_step((PATH), 0, ROOT, NAME_TEST);    \
    assert_no_predicate((PATH), 0)

#define assert_name_step(PATH,INDEX, NAME, EXPECTED_STEP_KIND)      \
    assert_step((PATH), (INDEX), (EXPECTED_STEP_KIND), NAME_TEST);   \
    assert_name(path_get((PATH), (INDEX)), (NAME))
#define assert_single_name_step(PATH, INDEX, NAME) assert_name_step((PATH), (INDEX), (NAME), SINGLE)
#define assert_recursive_name_step(PATH, INDEX, NAME) assert_name_step((PATH), (INDEX), (NAME), RECURSIVE)

#define assert_wildcard_step(PATH, INDEX, EXPECTED_STEP_KIND) assert_step((PATH), (INDEX), (EXPECTED_STEP_KIND), WILDCARD_TEST)
#define assert_single_wildcard_step(PATH, INDEX) assert_wildcard_step((PATH), (INDEX), SINGLE)
#define assert_recursive_wildcard_step(PATH, INDEX) assert_wildcard_step((PATH), (INDEX), RECURSIVE)

#define assert_type_kind(STEP, EXPECTED) assert_int_eq((EXPECTED), type_test_step_kind((STEP)))

#define assert_type_step(PATH, INDEX, EXPECTED_TYPE_KIND, EXPECTED_STEP_KIND) \
    assert_step((PATH), (INDEX), (EXPECTED_STEP_KIND), TYPE_TEST);       \
    assert_type_kind(path_get((PATH), INDEX), (EXPECTED_TYPE_KIND))

#define assert_single_type_step(PATH, INDEX, EXPECTED_TYPE_KIND)    \
    assert_type_step((PATH), (INDEX), (EXPECTED_TYPE_KIND), SINGLE)
#define assert_recursive_type_step(PATH, INDEX, EXPECTED_TYPE_KIND) \
    assert_type_step((PATH), (INDEX), (EXPECTED_TYPE_KIND), RECURSIVE)

#define assert_predicate_kind(PREDICATE, EXPECTED) assert_int_eq((EXPECTED), predicate_kind((PREDICATE)))

#define assert_predicate(PATH, PATH_INDEX, EXPECTED_PREDICATE_KIND)     \
    assert_true(step_has_predicate(path_get((PATH), (PATH_INDEX))));    \
    assert_not_null(path_get((PATH), (PATH_INDEX))->predicate);         \
    assert_not_null(step_predicate(path_get((PATH), (PATH_INDEX))));    \
    assert_predicate_kind(step_predicate(path_get((PATH), (PATH_INDEX))), (EXPECTED_PREDICATE_KIND))

#define assert_wildcard_predicate(PATH, PATH_INDEX) assert_predicate((PATH), (PATH_INDEX), (WILDCARD))

#define assert_subscript_index(PREDICATE, VALUE) assert_uint_eq((VALUE), subscript_predicate_index((PREDICATE)))
#define assert_subscript_predicate(PATH, PATH_INDEX, INDEX_VALUE)       \
    assert_predicate((PATH), (PATH_INDEX), SUBSCRIPT);                   \
    assert_subscript_index(step_predicate(path_get((PATH), (PATH_INDEX))), (INDEX_VALUE));

#define assert_slice_from(PREDICATE, VALUE) assert_int_eq((VALUE), slice_predicate_from((PREDICATE)))
#define assert_slice_to(PREDICATE, VALUE) assert_int_eq((VALUE), slice_predicate_to((PREDICATE)))
#define assert_slice_step(PREDICATE, VALUE) assert_int_eq((VALUE), slice_predicate_step((PREDICATE)))
#define assert_slice_predicate(PATH, PATH_INDEX, FROM_VALUE, TO_VALUE, STEP_VALUE) \
    assert_predicate((PATH), (PATH_INDEX), (SLICE));                     \
    assert_slice_from(step_predicate(path_get((PATH), (PATH_INDEX))), (FROM_VALUE)); \
    assert_slice_to(step_predicate(path_get((PATH), (PATH_INDEX))), (TO_VALUE)); \
    assert_slice_step(step_predicate(path_get(((PATH)), (PATH_INDEX))), (STEP_VALUE))

START_TEST (null_expression)
{
    char *expression = NULL;
    MaybeJsonPath maybe = parse((uint8_t *)expression, 50);

    assert_parser_failure(expression, maybe, ERR_NULL_EXPRESSION, 0);
    path_free(maybe);
}
END_TEST

START_TEST (zero_length)
{
    char *expression = "";
    MaybeJsonPath maybe = parse((uint8_t *)expression, 0);

    assert_parser_failure(expression, maybe, ERR_ZERO_LENGTH, 0);
    path_free(maybe);
}
END_TEST

START_TEST (missing_step_test)
{
    char *expression = "$.";
    MaybeJsonPath maybe = parse((uint8_t *)expression, 2);
    
    assert_parser_failure(expression, maybe, ERR_PREMATURE_END_OF_INPUT, 2);
    path_free(maybe);
}
END_TEST

START_TEST (missing_recursive_step_test)
{
    char *expression = "$..";
    MaybeJsonPath maybe = parse((uint8_t *)expression, 3);
    
    assert_parser_failure(expression, maybe, ERR_PREMATURE_END_OF_INPUT, 3);
    path_free(maybe);
}
END_TEST

START_TEST (missing_dot)
{
    char *expression = "$x";
    reset_errno();
    MaybeJsonPath maybe = parse((uint8_t *)expression, 2);
    
    assert_parser_failure(expression, maybe, ERR_UNEXPECTED_VALUE, 1);
    path_free(maybe);
}
END_TEST

/*
START_TEST (relative_path_begins_with_dot)
{
    char *expression = ".x";
    MaybeJsonPath maybe = parse((uint8_t *)expression, 2);
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NAME_CHAR, 0);
    path_free(maybe);
}
END_TEST

START_TEST (quoted_empty_step)
{
    char *expression = "$.foo.''.bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NAME_CHAR, 7);
    
    path_free(maybe);
}
END_TEST

START_TEST (empty_predicate)
{
    char *expression = "$.foo[].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EMPTY_PREDICATE, 6);
    path_free(maybe);
}
END_TEST

START_TEST (extra_junk_in_predicate)
{
    char *expression = "$.foo[ * quux].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXTRA_JUNK_AFTER_PREDICATE, 9);
    path_free(maybe);
}
END_TEST

START_TEST (whitespace_predicate)
{
    char *expression = "$.foo[ \t ].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EMPTY_PREDICATE, 9);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_predicate)
{
    char *expression = "$.foo[asdf].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_UNSUPPORTED_PRED_TYPE, 6);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_type_test_name)
{
    char *expression = "$.foo.monkey()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_oblong)
{
    char *expression = "$.foo.oblong()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_alloy)
{
    char *expression = "$.foo.alloy()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_strong)
{
    char *expression = "$.foo.strong()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_numbered)
{
    char *expression = "$.foo.numbered()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_booger)
{
    char *expression = "$.foo.booger()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_narl)
{
    char *expression = "$.foo.narl()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (empty_type_test_name)
{
    char *expression = "$.foo.()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_failure(expression, maybe, ERR_EXPECTED_NODE_TYPE_TEST, 6);
    path_free(maybe);
}
END_TEST

START_TEST (dollar_only)
{
    char *expression = "$";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 1);
    assert_root_step(maybe.value);

    path_free(maybe);
}
END_TEST

START_TEST (absolute_single_step)
{
    char *expression = "$.foo";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 2);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_no_predicate(maybe.value, 0);

    path_free(maybe);
}
END_TEST

START_TEST (absolute_recursive_step)
{
    char *expression = "$..foo";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 2);
    assert_root_step(maybe.value);
    assert_recursive_name_step(maybe.value, 1, "foo");
    assert_no_predicate(maybe.value, 0);

    path_free(maybe);
}
END_TEST

START_TEST (absolute_multi_step)
{
    char *expression = "$.foo.baz..yobble.thingum";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 5);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_name_step(maybe.value, 2, "baz");
    assert_recursive_name_step(maybe.value, 3, "yobble");
    assert_single_name_step(maybe.value, 4, "thingum");
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);
    assert_no_predicate(maybe.value, 3);
    assert_no_predicate(maybe.value, 4);

    path_free(maybe);
}
END_TEST

START_TEST (relative_multi_step)
{
    char *expression = "foo.bar..baz";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, RELATIVE_PATH, 3);
    assert_single_name_step(maybe.value, 0, "foo");
    assert_single_name_step(maybe.value, 1, "bar");
    assert_recursive_name_step(maybe.value, 2, "baz");
    assert_no_predicate(maybe.value, 0);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (quoted_multi_step)
{
    char *expression = "$.foo.'happy fun ball'.bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 4);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_name_step(maybe.value, 2, "happy fun ball");
    assert_single_name_step(maybe.value, 3, "bar");
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);
    assert_no_predicate(maybe.value, 3);

    path_free(maybe);
}
END_TEST

START_TEST (wildcard)
{
    char *expression = "$.foo.*";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_wildcard_step(maybe.value, 2);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (recursive_wildcard)
{
    char *expression = "$.foo..*";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_recursive_wildcard_step(maybe.value, 2);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (wildcard_with_subscript_predicate)
{
    char *expression = "$.foo.* [0]";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));

    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_no_predicate(maybe.value, 1);
    assert_single_wildcard_step(maybe.value, 2);
    assert_subscript_predicate(maybe.value, 2, 0);

    path_free(maybe);
}
END_TEST

START_TEST (whitespace)
{
    char *expression = "  $ \r\n. foo \n.\n. \t'happy fun ball' . \t string()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 4);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_recursive_name_step(maybe.value, 2, "happy fun ball");
    assert_single_type_step(maybe.value, 3, STRING_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);
    assert_no_predicate(maybe.value, 3);

    path_free(maybe);
}
END_TEST

START_TEST (type_test_missing_closing_paren)
{
    char *expression = "$.foo.null(";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_name_step(maybe.value, 2, "null(");
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (recursive_type_test)
{
    char *expression = "$.foo..string()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_recursive_type_step(maybe.value, 2, STRING_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (object_type_test)
{
    char *expression = "$.foo.object()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_type_step(maybe.value, 2, OBJECT_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (array_type_test)
{
    char *expression = "$.foo.array()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_type_step(maybe.value, 2, ARRAY_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (string_type_test)
{
    char *expression = "$.foo.string()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_type_step(maybe.value, 2, STRING_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (number_type_test)
{
    char *expression = "$.foo.number()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_type_step(maybe.value, 2, NUMBER_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (boolean_type_test)
{
    char *expression = "$.foo.boolean()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_type_step(maybe.value, 2, BOOLEAN_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (null_type_test)
{
    char *expression = "$.foo.null()";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_single_type_step(maybe.value, 2, NULL_TEST);
    assert_no_predicate(maybe.value, 1);
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (wildcard_predicate)
{
    char *expression = "$.store.book[*].author";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 4);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "store");
    assert_no_predicate(maybe.value, 1);
    assert_single_name_step(maybe.value, 2, "book");
    assert_wildcard_predicate(maybe.value, 2);
    assert_single_name_step(maybe.value, 3, "author");
    assert_no_predicate(maybe.value, 3);

    path_free(maybe);
}
END_TEST

START_TEST (wildcard_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t*\n]  .bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_wildcard_predicate(maybe.value, 1);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.foo[42].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_subscript_predicate(maybe.value, 1, 42);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (subscript_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t42\r]\n.bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_subscript_predicate(maybe.value, 1, 42);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (type_test_with_subscript_predicate)
{
    char *expression = "$.foo.array()[0]";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_no_predicate(maybe.value, 1);
    assert_single_type_step(maybe.value, 2, ARRAY_TEST);
    assert_subscript_predicate(maybe.value, 2, 0);

    path_free(maybe);
}
END_TEST

START_TEST (negative_subscript_predicate)
{
    char *expression = "$.foo[ -3].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    // xxx - fixme! this should be ERR_EXPECTED_INTEGER instead!
    assert_parser_failure(expression, maybe, ERR_UNSUPPORTED_PRED_TYPE, 7);
    path_free(maybe);
}
END_TEST

START_TEST (slice_predicate_form1)
{
    char *expression = "$.foo[:-3].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, INT_FAST32_MIN, -3, 1);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (slice_predicate_form1_with_step)
{
    char *expression = "$.foo[:-3:2].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, INT_FAST32_MIN, -3, 2);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (slice_predicate_form2)
{
    char *expression = "$.foo[-3:].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, -3, INT_FAST32_MAX, 1);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (slice_predicate_form2_with_step)
{
    char *expression = "$.foo[-1::2].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, -1, INT_FAST32_MAX, 2);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (slice_predicate_form3)
{
    char *expression = "$.foo[3:5].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, 3, 5, 1);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (slice_predicate_form3_with_step)
{
    char *expression = "$.foo[1:4:2].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, 1, 4, 2);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (slice_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t1\t:\t5\r:\n3\t]\n.bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, 1, 5, 3);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (negative_step_slice_predicate)
{
    char *expression = "$.foo[1:3:-3].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);
    assert_root_step(maybe.value);
    assert_single_name_step(maybe.value, 1, "foo");
    assert_slice_predicate(maybe.value, 1, 1, 3, -3);
    assert_single_name_step(maybe.value, 2, "bar");
    assert_no_predicate(maybe.value, 2);

    path_free(maybe);
}
END_TEST

START_TEST (zero_step_slice_predicate)
{
    char *expression = "$.foo[::0].bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    // xxx - fix me! this should be ERR_STEP_CANNOT_BE_ZERO instead
    // xxx - fix me! this should be position 8 instead, need a non-zero signed int parser
    assert_parser_failure(expression, maybe, ERR_UNSUPPORTED_PRED_TYPE, 9);

    path_free(maybe);
}
END_TEST

static bool count(Step *each __attribute__((unused)), void *context)
{
    unsigned long *counter = (unsigned long *)context;
    (*counter)++;
    return true;
}

START_TEST (iteration)
{
    char *expression = "$.foo.bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);

    unsigned long counter = 0;
    assert_true(path_iterate(maybe.value, count, &counter));
    assert_uint_eq(3, counter);

    path_free(maybe);
}
END_TEST

static bool fail_count(Step *each __attribute__((unused)), void *context)
{
    unsigned long *counter = (unsigned long *)context;
    if(0 == *counter)
    {
        (*counter)++;
        return true;
    }
    return false;
}

START_TEST (fail_iteration)
{
    char *expression = "$.foo.bar";
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);

    unsigned long counter = 0;
    assert_false(path_iterate(maybe.value, fail_count, &counter));
    assert_uint_eq(1, counter);

    path_free(maybe);
}
END_TEST

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
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));

    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 1);

    reset_errno();
    assert_null(path_get(maybe.value, 1));
    assert_errno(EINVAL);

    path_free(maybe);
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
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));

    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);

    Step *step2 = path_get(maybe.value, 2);
    reset_errno();
    assert_uint_eq(0, name_test_step_length(step2));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(name_test_step_name(step2));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(step_predicate(step2));
    assert_errno(EINVAL);

    path_free(maybe);
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
    MaybeJsonPath maybe = parse((uint8_t *)expression, strlen(expression));
    assert_parser_success(expression, maybe, ABSOLUTE_PATH, 3);

    Predicate *subscript = step_predicate(path_get(maybe.value, 1));
    reset_errno();
    assert_int_eq(0, slice_predicate_to(subscript));
    assert_errno(EINVAL);
    reset_errno();
    assert_int_eq(0, slice_predicate_from(subscript));
    assert_errno(EINVAL);
    reset_errno();
    assert_int_eq(0, slice_predicate_step(subscript));
    assert_errno(EINVAL);

    Predicate *wildcard_pred = step_predicate(path_get(maybe.value, 2));
    reset_errno();
    assert_uint_eq(0, subscript_predicate_index(wildcard_pred));
    assert_errno(EINVAL);
    reset_errno();
    assert_null(join_predicate_left(wildcard_pred));
    assert_errno(EINVAL);
    reset_errno();
    assert_null(join_predicate_right(wildcard_pred));
    assert_errno(EINVAL);

    path_free(maybe);
}
END_TEST
*/

Suite *jsonpath_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_expression);
    tcase_add_test(bad_input_case, zero_length);
    tcase_add_test(bad_input_case, missing_step_test);
    tcase_add_test(bad_input_case, missing_recursive_step_test);
    tcase_add_test(bad_input_case, missing_dot);
    /*
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
    */

    Suite *suite = suite_create("Parser");
    suite_add_tcase(suite, bad_input_case);
    /*
    suite_add_tcase(suite, basic_case);
    suite_add_tcase(suite, node_type_case);
    suite_add_tcase(suite, predicate_case);
    suite_add_tcase(suite, api_case);
    */

    return suite;
}
