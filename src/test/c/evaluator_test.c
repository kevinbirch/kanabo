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

#include "evaluator.h"
#include "loader.h"
#include "test.h"

static document_model *model = NULL;

void evaluator_setup(void);
void evaluator_teardown(void);

void evaluator_setup(void)
{
    model = make_model(1);
    ck_assert_not_null(model);
    
    FILE *input = fopen("inventory.json", "r");
    ck_assert_not_null(input);
    
    loader_result *result = load_model_from_file(input, model);
    ck_assert_not_null(result);
    ck_assert_int_eq(LOADER_SUCCESS, result->code);

    ck_assert_int_eq(0, fclose(input));
    free_loader_result(result);
}

void evaluator_teardown(void)
{
    model_free(model);
    model = NULL;
}

START_TEST (dollar_only)
{
    jsonpath path;
    jsonpath_status_code code = parse_jsonpath((uint8_t *)"$", 1, &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));
    ck_assert_int_eq(MAPPING, node_get_kind(nodelist_get(list, 0)));
    ck_assert_int_eq(1, node_get_size(nodelist_get(list, 0)));
    ck_assert_true(mapping_contains_key(nodelist_get(list, 0), "store"));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (single_name_step)
{
    jsonpath path;
    jsonpath_status_code code = parse_jsonpath((uint8_t *)"$.store", 7, &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));
    node *store = nodelist_get(list, 0);
    ck_assert_not_null(store);

    ck_assert_int_eq(MAPPING, node_get_kind(store));
    ck_assert_int_eq(2, node_get_size(store));
    ck_assert_true(mapping_contains_key(store, "book"));
    ck_assert_true(mapping_contains_key(store, "bicycle"));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (long_path)
{
    jsonpath path;
    char *expr = "$.store.bicycle.color";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));
    node *color = nodelist_get(list, 0);
    ck_assert_not_null(color);

    ck_assert_int_eq(SCALAR, node_get_kind(color));
    ck_assert_buf_eq("red", 3, scalar_get_value(color), node_get_size(color));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard)
{
    jsonpath path;
    char *expr = "$.store.*";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(5, nodelist_length(list));
    node *store = nodelist_get(list, 0);
    ck_assert_not_null(store);

    ck_assert_int_eq(MAPPING, node_get_kind(nodelist_get(list, 0)));
    ck_assert_int_eq(MAPPING, node_get_kind(nodelist_get(list, 1)));
    ck_assert_int_eq(MAPPING, node_get_kind(nodelist_get(list, 2)));
    ck_assert_int_eq(MAPPING, node_get_kind(nodelist_get(list, 3)));
    ck_assert_int_eq(MAPPING, node_get_kind(nodelist_get(list, 4)));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (object_test)
{
    jsonpath path;
    char *expr = "$.store.object()";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));
    node *boolean = nodelist_get(list, 0);
    ck_assert_not_null(boolean);

    ck_assert_int_eq(SCALAR, node_get_kind(boolean));
    ck_assert_buf_eq("true", 4, scalar_get_value(boolean), node_get_size(boolean));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (array_test)
{
    jsonpath path;
    char * expr = "$.store.book.array()";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));
    node *boolean = nodelist_get(list, 0);
    ck_assert_not_null(boolean);

    ck_assert_int_eq(SCALAR, node_get_kind(boolean));
    ck_assert_buf_eq("true", 4, scalar_get_value(boolean), node_get_size(boolean));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_predicate)
{
    jsonpath path;
    char *expr = "$.store.book[*].author";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(4, nodelist_length(list));

    ck_assert_int_eq(SCALAR, node_get_kind(nodelist_get(list, 0)));
    ck_assert_int_eq(SCALAR, node_get_kind(nodelist_get(list, 1)));
    ck_assert_int_eq(SCALAR, node_get_kind(nodelist_get(list, 2)));
    ck_assert_int_eq(SCALAR, node_get_kind(nodelist_get(list, 3)));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_predicate_on_mapping)
{
    jsonpath path;
    char *expr = "$.store.bicycle[*].color";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));

    node *scalar = nodelist_get(list, 0);
    ck_assert_int_eq(SCALAR, node_get_kind(scalar));
    ck_assert_buf_eq("red", 3, scalar_get_value(scalar), node_get_size(scalar));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (wildcard_predicate_on_scalar)
{
    jsonpath path;
    char *expr = "$.store.bicycle.color[*]";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));

    node *scalar = nodelist_get(list, 0);
    ck_assert_int_eq(SCALAR, node_get_kind(scalar));
    ck_assert_buf_eq("red", 3, scalar_get_value(scalar), node_get_size(scalar));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

START_TEST (subscript_predicate)
{
    jsonpath path;
    char *expr = "$.store.book[2]";
    jsonpath_status_code code = parse_jsonpath((uint8_t *)expr, strlen(expr), &path);
    ck_assert_int_eq(JSONPATH_SUCCESS, code);

    errno = 0;
    nodelist *list = evaluate(model, &path);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(list);
    ck_assert_int_eq(1, nodelist_length(list));

    errno = 0;
    node *mapping = nodelist_get(list, 0);
    ck_assert_int_eq(MAPPING, node_get_kind(mapping));
    ck_assert_int_eq(0, errno);
    errno = 0;
    node *author = mapping_get_value(mapping, "author");
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(author);
    char *melville = "Herman Melville";
    ck_assert_buf_eq(melville, strlen(melville), scalar_get_value(author), node_get_size(author));

    nodelist_free(list);
    jsonpath_free(&path);
}
END_TEST

Suite *evaluator_suite(void)
{
    TCase *basic = tcase_create("basic");
    tcase_add_checked_fixture(basic, evaluator_setup, evaluator_teardown);
    tcase_add_test(basic, dollar_only);
    tcase_add_test(basic, single_name_step);
    tcase_add_test(basic, long_path);
    tcase_add_test(basic, wildcard);
    tcase_add_test(basic, object_test);
    tcase_add_test(basic, array_test);

    TCase *predicate = tcase_create("predicate");
    tcase_add_checked_fixture(predicate, evaluator_setup, evaluator_teardown);
    tcase_add_test(predicate, wildcard_predicate);
    tcase_add_test(predicate, wildcard_predicate_on_mapping);
    tcase_add_test(predicate, wildcard_predicate_on_scalar);
    tcase_add_test(predicate, subscript_predicate);
    
    Suite *evaluator = suite_create("Evaluator");
    suite_add_tcase(evaluator, basic);
    suite_add_tcase(evaluator, predicate);

    return evaluator;
}

