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
#include "evaluator/private.h"
#undef component_name
#include "jsonpath/private.h"
#undef component_name
#include "loader.h"
#include "test.h"
#include "test_model.h"
#include "test_nodelist.h"


static document_model *model_fixture = NULL;

#define assert_evaluator_failure(CONTEXT, CODE)                         \
    do                                                                  \
    {                                                                   \
        assert_int_eq((CODE), (CONTEXT).nothing.code);                  \
        log_debug("evaluator test", "received expected failure message: '%s'", evaluator_status_message((CODE))); \
    } while(0)

static document_model *load_document(const char *filename)
{
    assert_not_null(filename);
    FILE *input = fopen(filename, "r");
    assert_not_null(input);

    reset_errno();
    MaybeDocument maybe = load_file(input, DUPE_CLOBBER);
    assert_noerr();
    int result = fclose(input);
    assert_int_eq(0, result);

    assert_int_eq(JUST, maybe.tag);
    assert_not_null(maybe.just);

    return maybe.just;
}

static void inventory_setup(void)
{
    model_fixture = load_document("inventory.json");
}

static void invoice_setup(void)
{
    model_fixture = load_document("invoice.yaml");
}

static void evaluator_teardown(void)
{
    model_free(model_fixture);
    model_fixture = NULL;
}

START_TEST (null_model)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    reset_errno();
    MaybeNodelist maybe = evaluate(NULL, path);
    assert_errno(EINVAL);
    assert_evaluator_failure(maybe, ERR_MODEL_IS_NULL);

    path_free(path);
}
END_TEST

START_TEST (null_path)
{
    document_model *bad_model = make_model();

    reset_errno();
    MaybeNodelist maybe = evaluate(bad_model, NULL);
    assert_errno(EINVAL);
    assert_evaluator_failure(maybe, ERR_PATH_IS_NULL);

    model_free(bad_model);
}
END_TEST

START_TEST (null_document)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    document_model *bad_model = make_model();

    reset_errno();
    MaybeNodelist maybe = evaluate(bad_model, path);
    assert_errno(EINVAL);
    assert_evaluator_failure(maybe, ERR_NO_DOCUMENT_IN_MODEL);

    model_free(bad_model);
    path_free(path);
}
END_TEST

START_TEST (null_document_root)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    document_model *bad_model = make_model();
    node *document = (node *)calloc(1, sizeof(struct node));
    assert_not_null(document);
    document->tag.kind = DOCUMENT;
    document->tag.name = NULL;
    document->content.size = 0;
    document->content.target = NULL;
    model_add(bad_model, document);

    reset_errno();
    MaybeNodelist maybe = evaluate(bad_model, path);
    assert_errno(EINVAL);
    assert_evaluator_failure(maybe, ERR_NO_ROOT_IN_DOCUMENT);

    model_free(bad_model);
    path_free(path);
}
END_TEST

START_TEST (relative_path)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    document_model *bad_model = make_model();
    node *root = make_mapping_node();
    node *document = make_document_node();
    document_set_root(document, root);
    model_add(bad_model, document);

    reset_errno();
    MaybeNodelist maybe = evaluate(bad_model, path);
    assert_errno(EINVAL);
    assert_evaluator_failure(maybe, ERR_PATH_IS_NOT_ABSOLUTE);

    model_free(bad_model);
    path_free(path);
}
END_TEST


START_TEST (empty_path)
{
    jsonpath *path = (jsonpath *)calloc(1, sizeof(jsonpath));
    assert_not_null(path);
    path->kind = ABSOLUTE_PATH;
    path->length = 0;
    path->steps = NULL;

    document_model *bad_model = make_model();
    node *root = make_mapping_node();
    node *document = make_document_node();
    document_set_root(document, root);
    model_add(bad_model, document);

    reset_errno();
    MaybeNodelist maybe = evaluate(bad_model, path);
    assert_errno(EINVAL);
    assert_evaluator_failure(maybe, ERR_PATH_IS_EMPTY);

    model_free(bad_model);
    free(path);
}
END_TEST

static nodelist *evaluate_expression(const char *expression)
{
    parser_context *parser = make_parser((const uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    reset_errno();
    MaybeNodelist maybe = evaluate(model_fixture, path);
    assert_noerr();
    assert_int_eq(JUST, maybe.tag);
    assert_not_null(maybe.just);
    path_free(path);
    return maybe.just;
}

START_TEST (dollar_only)
{
    nodelist *list = evaluate_expression("$");

    assert_nodelist_length(list, 1);
    assert_node_kind(nodelist_get(list, 0), MAPPING);
    assert_node_size(nodelist_get(list, 0), 1);
    assert_mapping_has_key(nodelist_get(list, 0), "store");

    nodelist_free(list);
}
END_TEST

START_TEST (single_name_step)
{
    nodelist *list = evaluate_expression("$.store");

    assert_nodelist_length(list, 1);
    node *store = nodelist_get(list, 0);
    assert_not_null(store);

    assert_node_kind(store, MAPPING);
    assert_node_size(store, 2);
    assert_mapping_has_key(store, "book");
    assert_mapping_has_key(store, "bicycle");

    nodelist_free(list);
}
END_TEST

START_TEST (simple_recursive_step)
{
    nodelist *list = evaluate_expression("$..author");

    assert_nodelist_length(list, 5);

    assert_node_kind(nodelist_get(list, 0), SCALAR);
    assert_node_kind(nodelist_get(list, 1), SCALAR);
    assert_node_kind(nodelist_get(list, 2), SCALAR);
    assert_node_kind(nodelist_get(list, 3), SCALAR);

    nodelist_free(list);
}
END_TEST

START_TEST (compound_recursive_step)
{
    nodelist *list = evaluate_expression("$.store..price");

    assert_nodelist_length(list, 6);

    assert_scalar_kind(nodelist_get(list, 0), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 1), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 2), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 3), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 4), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 5), SCALAR_REAL);

    nodelist_free(list);
}
END_TEST

START_TEST (long_path)
{
    nodelist *list = evaluate_expression("$.store.bicycle.color");

    assert_nodelist_length(list, 1);
    node *color = nodelist_get(list, 0);
    assert_not_null(color);

    assert_node_kind(color, SCALAR);
    assert_scalar_value(color, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard)
{
    nodelist *list = evaluate_expression("$.store.*");

    assert_nodelist_length(list, 6);

    assert_node_kind(nodelist_get(list, 0), MAPPING);
    assert_node_kind(nodelist_get(list, 1), MAPPING);
    assert_node_kind(nodelist_get(list, 2), MAPPING);
    assert_node_kind(nodelist_get(list, 3), MAPPING);
    assert_node_kind(nodelist_get(list, 4), MAPPING);

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_wildcard)
{
    nodelist *list = evaluate_expression("$..*");

    assert_nodelist_length(list, 34);

    assert_node_kind(nodelist_get(list, 0), MAPPING);

    nodelist_free(list);
}
END_TEST

START_TEST (object_test)
{
    nodelist *list = evaluate_expression("$.store.object()");

    assert_nodelist_length(list, 1);

    node *value = nodelist_get(list, 0);
    assert_not_null(value);

    assert_node_kind(value, MAPPING);
    assert_mapping_has_key(value, "book");
    assert_mapping_has_key(value, "bicycle");

    nodelist_free(list);
}
END_TEST

START_TEST (array_test)
{
    nodelist *list = evaluate_expression("$.store.book.array()");

    assert_nodelist_length(list, 1);

    node *value = nodelist_get(list, 0);
    assert_not_null(value);
    assert_node_kind(value, SEQUENCE);
    assert_node_size(value, 5);

    nodelist_free(list);
}
END_TEST

START_TEST (number_test)
{
    nodelist *list = evaluate_expression("$.store.book[*].price.number()");

    assert_nodelist_length(list, 5);

    assert_scalar_value(nodelist_get(list, 0), "8.95");
    assert_scalar_value(nodelist_get(list, 1), "12.99");
    assert_scalar_value(nodelist_get(list, 2), "8.99");
    assert_scalar_value(nodelist_get(list, 3), "22.99");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate)
{
    nodelist *list = evaluate_expression("$.store.book[*].author");

    assert_nodelist_length(list, 5);

    assert_node_kind(nodelist_get(list, 0), SCALAR);
    assert_node_kind(nodelist_get(list, 1), SCALAR);
    assert_node_kind(nodelist_get(list, 2), SCALAR);
    assert_node_kind(nodelist_get(list, 3), SCALAR);

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate_on_mapping)
{
    nodelist *list = evaluate_expression("$.store.bicycle[*].color");

    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate_on_scalar)
{
    nodelist *list = evaluate_expression("$.store.bicycle.color[*]");

    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (subscript_predicate)
{
    nodelist *list = evaluate_expression("$.store.book[2]");

    assert_nodelist_length(list, 1);

    reset_errno();
    node *mapping = nodelist_get(list, 0);
    assert_node_kind(mapping, MAPPING);
    assert_noerr();
    reset_errno();
    node *author = mapping_get(mapping, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Herman Melville");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_subscript_predicate)
{
    nodelist *list = evaluate_expression("$..book[2]");

    assert_nodelist_length(list, 1);

    reset_errno();
    node *mapping = nodelist_get(list, 0);
    assert_node_kind(mapping, MAPPING);
    assert_noerr();
    reset_errno();
    node *author = mapping_get(mapping, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Herman Melville");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate)
{
    nodelist *list = evaluate_expression("$.store.book[:2]");

    assert_nodelist_length(list, 2);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    reset_errno();
    book = nodelist_get(list, 1);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Evelyn Waugh");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_slice_predicate)
{
    nodelist *list = evaluate_expression("$..book[:2]");

    assert_nodelist_length(list, 2);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    reset_errno();
    book = nodelist_get(list, 1);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Evelyn Waugh");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_with_step)
{
    nodelist *list = evaluate_expression("$.store.book[:2:2]");

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_negative_from)
{
    nodelist *list = evaluate_expression("$.store.book[-1:]");

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "夏目漱石 (NATSUME Sōseki)");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_slice_predicate_negative_from)
{
    nodelist *list = evaluate_expression("$..book[-1:]");

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "夏目漱石 (NATSUME Sōseki)");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_copy)
{
    nodelist *list = evaluate_expression("$.store.book[::]");

    assert_nodelist_length(list, 5);

    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 0), (uint8_t *)"author", 6ul), "Nigel Rees");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 1), (uint8_t *)"author", 6ul), "Evelyn Waugh");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 2), (uint8_t *)"author", 6ul), "Herman Melville");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 3), (uint8_t *)"author", 6ul), "J. R. R. Tolkien");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_reverse)
{
    nodelist *list = evaluate_expression("$.store.book[::-1]");

    assert_nodelist_length(list, 5);

    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 0), (uint8_t *)"author", 6ul), "夏目漱石 (NATSUME Sōseki)");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 1), (uint8_t *)"author", 6ul), "J. R. R. Tolkien");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 2), (uint8_t *)"author", 6ul), "Herman Melville");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 3), (uint8_t *)"author", 6ul), "Evelyn Waugh");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 4), (uint8_t *)"author", 6ul), "Nigel Rees");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (name_alias)
{
    nodelist *list = evaluate_expression("$.payment.billing-address.name");

    assert_nodelist_length(list, 1);
    reset_errno();
    assert_scalar_value(nodelist_get(list, 0), "Ramond Hessel");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (type_alias)
{
    nodelist *list = evaluate_expression("$.shipments[0].*.number()");

    assert_nodelist_length(list, 1);
    reset_errno();
    assert_scalar_value(nodelist_get(list, 0), "237.23");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (greedy_wildcard_alias)
{
    nodelist *list = evaluate_expression("$.shipments[0].items.*");

    assert_nodelist_length(list, 2);

    reset_errno();
    node *zero = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(zero, MAPPING);
    assert_scalar_value(mapping_get(zero, (uint8_t *)"isbn", 4), "1428312250");

    node *one = nodelist_get(list, 1);
    assert_noerr();
    assert_node_kind(one, MAPPING);
    assert_scalar_value(mapping_get(one, (uint8_t *)"isbn", 4), "0323073867");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_alias)
{
    nodelist *list = evaluate_expression("$.shipments..isbn");

    assert_nodelist_length(list, 2);

    assert_scalar_value(nodelist_get(list, 0), "1428312250");
    assert_scalar_value(nodelist_get(list, 1), "0323073867");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate_alias)
{
    nodelist *list = evaluate_expression("$.shipments[0].items[*].price");

    assert_nodelist_length(list, 2);

    assert_scalar_value(nodelist_get(list, 0), "135.48");
    assert_scalar_value(nodelist_get(list, 1), "84.18");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_wildcard_alias)
{
    nodelist *list = evaluate_expression("$.shipments[0].items..*");

    assert_nodelist_length(list, 15);

    nodelist_free(list);
}
END_TEST

Suite *evaluator_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_model);
    tcase_add_test(bad_input_case, null_path);
    tcase_add_test(bad_input_case, null_document);
    tcase_add_test(bad_input_case, null_document_root);
    tcase_add_test(bad_input_case, relative_path);
    tcase_add_test(bad_input_case, empty_path);

    TCase *basic_case = tcase_create("basic");
    tcase_add_unchecked_fixture(basic_case, inventory_setup, evaluator_teardown);
    tcase_add_test(basic_case, dollar_only);
    tcase_add_test(basic_case, single_name_step);
    tcase_add_test(basic_case, long_path);
    tcase_add_test(basic_case, wildcard);
    tcase_add_test(basic_case, object_test);
    tcase_add_test(basic_case, array_test);
    tcase_add_test(basic_case, number_test);

    TCase *predicate_case = tcase_create("predicate");
    tcase_add_unchecked_fixture(predicate_case, inventory_setup, evaluator_teardown);
    tcase_add_test(predicate_case, wildcard_predicate);
    tcase_add_test(predicate_case, wildcard_predicate_on_mapping);
    tcase_add_test(predicate_case, wildcard_predicate_on_scalar);
    tcase_add_test(predicate_case, subscript_predicate);
    tcase_add_test(predicate_case, slice_predicate);
    tcase_add_test(predicate_case, subscript_predicate);
    tcase_add_test(predicate_case, slice_predicate_with_step);
    tcase_add_test(predicate_case, slice_predicate_negative_from);
    tcase_add_test(predicate_case, slice_predicate_copy);
    tcase_add_test(predicate_case, slice_predicate_reverse);

    TCase *recursive_case = tcase_create("recursive");
    tcase_add_unchecked_fixture(recursive_case, inventory_setup, evaluator_teardown);
    tcase_add_test(recursive_case, simple_recursive_step);
    tcase_add_test(recursive_case, compound_recursive_step);
    tcase_add_test(recursive_case, recursive_slice_predicate);
    tcase_add_test(recursive_case, recursive_subscript_predicate);
    tcase_add_test(recursive_case, recursive_slice_predicate_negative_from);
    tcase_add_test(recursive_case, recursive_wildcard);

    TCase *alias_case = tcase_create("alias");
    tcase_add_unchecked_fixture(alias_case, invoice_setup, evaluator_teardown);
    tcase_add_test(alias_case, name_alias);
    tcase_add_test(alias_case, type_alias);
    tcase_add_test(alias_case, greedy_wildcard_alias);
    tcase_add_test(alias_case, recursive_alias);
    tcase_add_test(alias_case, wildcard_predicate_alias);
    tcase_add_test(alias_case, recursive_wildcard_alias);

    Suite *evaluator = suite_create("Evaluator");
    suite_add_tcase(evaluator, bad_input_case);
    suite_add_tcase(evaluator, basic_case);
    suite_add_tcase(evaluator, predicate_case);
    suite_add_tcase(evaluator, recursive_case);
    suite_add_tcase(evaluator, alias_case);

    return evaluator;
}
