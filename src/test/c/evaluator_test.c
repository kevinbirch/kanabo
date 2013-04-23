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
#include "test_model.h"
#include "test_nodelist.h"

static document_model *model = NULL;

void evaluator_setup(void);
void evaluator_teardown(void);

bool scalar_true(node *each, void *context);

void evaluator_setup(void)
{
    model = make_model(1);
    assert_not_null(model);
    
    FILE *input = fopen("inventory.json", "r");
    assert_not_null(input);
    
    loader_result *result = load_model_from_file(input, model);
    assert_not_null(result);
    assert_int_eq(LOADER_SUCCESS, result->code);

    int closed = fclose(input);
    assert_int_eq(0, closed);
    free_loader_result(result);
}

void evaluator_teardown(void)
{
    model_free(model);
    model = NULL;
}

START_TEST (dollar_only)
{
    char *expression = "$";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);
    assert_node_kind(nodelist_get(list, 0), MAPPING);
    assert_node_size(nodelist_get(list, 0), 1);
    assert_mapping_has_key(nodelist_get(list, 0), "store");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (single_name_step)
{
    char *expression = "$.store";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);
    node *store = nodelist_get(list, 0);
    assert_not_null(store);

    assert_node_kind(store, MAPPING);
    assert_node_size(store, 2);
    assert_mapping_has_key(store, "book");
    assert_mapping_has_key(store, "bicycle");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (long_path)
{
    char *expression = "$.store.bicycle.color";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);
    node *color = nodelist_get(list, 0);
    assert_not_null(color);

    assert_node_kind(color, SCALAR);
    assert_scalar_value(color, "red");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (wildcard)
{
    char *expression = "$.store.*";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 5);

    assert_node_kind(nodelist_get(list, 0), MAPPING);
    assert_node_kind(nodelist_get(list, 1), MAPPING);
    assert_node_kind(nodelist_get(list, 2), MAPPING);
    assert_node_kind(nodelist_get(list, 3), MAPPING);
    assert_node_kind(nodelist_get(list, 4), MAPPING);

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (object_test)
{
    char *expression = "$.store.object()";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);

    node *value = nodelist_get(list, 0);
    assert_not_null(value);

    assert_node_kind(value, MAPPING);
    assert_mapping_has_key(value, "book");
    assert_mapping_has_key(value, "bicycle");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (array_test)
{
    char * expression = "$.store.book.array()";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);

    node *value = nodelist_get(list, 0);
    assert_not_null(value);
    assert_node_kind(value, SEQUENCE);
    assert_node_size(value, 4);

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (number_test)
{
    char * expression = "$.store.book[*].price.number()";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 4);

    assert_scalar_value(nodelist_get(list, 0), "8.95");
    assert_scalar_value(nodelist_get(list, 1), "12.99");
    assert_scalar_value(nodelist_get(list, 2), "8.99");
    assert_scalar_value(nodelist_get(list, 3), "22.99");
    
    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

bool scalar_true(node *each, void *context)
{
#pragma unused(context)

    return SCALAR == node_get_kind(each) &&
        SCALAR_BOOLEAN == scalar_get_kind(each) &&
        scalar_boolean_is_true(each);
}

START_TEST (wildcard_predicate)
{
    char *expression = "$.store.book[*].author";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 4);

    assert_node_kind(nodelist_get(list, 0), SCALAR);
    assert_node_kind(nodelist_get(list, 1), SCALAR);
    assert_node_kind(nodelist_get(list, 2), SCALAR);
    assert_node_kind(nodelist_get(list, 3), SCALAR);

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (wildcard_predicate_on_mapping)
{
    char *expression = "$.store.bicycle[*].color";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (wildcard_predicate_on_scalar)
{
    char *expression = "$.store.bicycle.color[*]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.store.book[2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);

    reset_errno();
    node *mapping = nodelist_get(list, 0);
    assert_node_kind(mapping, MAPPING);
    assert_noerr();
    reset_errno();
    node *author = mapping_get_value(mapping, "author");
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Herman Melville");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (slice_predicate)
{
    char *expression = "$.store.book[:2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 2);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get_value(book, "author");
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    reset_errno();
    book = nodelist_get(list, 1);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    author = mapping_get_value(book, "author");
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Evelyn Waugh");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (slice_predicate_with_step)
{
    char *expression = "$.store.book[:2:2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get_value(book, "author");
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (slice_predicate_negative_from)
{
    char *expression = "$.store.book[-1:]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get_value(book, "author");
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "J. R. R. Tolkien");

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (slice_predicate_copy)
{
    char *expression = "$.store.book[::]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 4);

    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 0), "author"), "Nigel Rees");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 1), "author"), "Evelyn Waugh");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 2), "author"), "Herman Melville");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 3), "author"), "J. R. R. Tolkien");
    assert_noerr();

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

START_TEST (slice_predicate_reverse)
{
    char *expression = "$.store.book[::-1]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);
    assert_noerr();

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));

    evaluator_context *context = make_evaluator(model, path);
    assert_not_null(context);

    reset_errno();
    nodelist *list = evaluate(context);
    assert_noerr();
    assert_not_null(list);
    assert_nodelist_length(list, 4);

    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 0), "author"), "J. R. R. Tolkien");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 1), "author"), "Herman Melville");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 2), "author"), "Evelyn Waugh");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 3), "author"), "Nigel Rees");
    assert_noerr();

    evaluator_free(context);
    nodelist_free(list);
    parser_free(parser);
    path_free(path);
}
END_TEST

Suite *evaluator_suite(void)
{
    TCase *basic_case = tcase_create("basic");
    tcase_add_checked_fixture(basic_case, evaluator_setup, evaluator_teardown);
    tcase_add_test(basic_case, dollar_only);
    tcase_add_test(basic_case, single_name_step);
    tcase_add_test(basic_case, long_path);
    tcase_add_test(basic_case, wildcard);
    tcase_add_test(basic_case, object_test);
    tcase_add_test(basic_case, array_test);
    tcase_add_test(basic_case, number_test);

    TCase *predicate_case = tcase_create("predicate");
    tcase_add_checked_fixture(predicate_case, evaluator_setup, evaluator_teardown);
    tcase_add_test(predicate_case, wildcard_predicate);
    tcase_add_test(predicate_case, wildcard_predicate_on_mapping);
    tcase_add_test(predicate_case, wildcard_predicate_on_scalar);
    tcase_add_test(predicate_case, subscript_predicate);
    tcase_add_test(predicate_case, slice_predicate);
    tcase_add_test(predicate_case, slice_predicate_with_step);
    tcase_add_test(predicate_case, slice_predicate_negative_from);
    tcase_add_test(predicate_case, slice_predicate_copy);
    tcase_add_test(predicate_case, slice_predicate_reverse);

    Suite *evaluator = suite_create("Evaluator");
    suite_add_tcase(evaluator, basic_case);
    suite_add_tcase(evaluator, predicate_case);

    return evaluator;
}

