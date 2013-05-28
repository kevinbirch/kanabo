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

void evaluator_setup(void);
void evaluator_teardown(void);

bool scalar_true(node *each, void *context);

static document_model *model = NULL;

#define assert_evaluator_failure(CONTEXT, EXPECTED_RESULT)              \
    do                                                                  \
    {                                                                   \
        assert_not_null((CONTEXT));                                     \
        assert_int_eq((EXPECTED_RESULT), evaluator_status((CONTEXT)));  \
        log_debug("evaluator test", "received expected failure message: '%s'", evaluator_status_message((CONTEXT))); \
    } while(0)

START_TEST (null_model)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(NULL, path);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_MODEL_IS_NULL);
    path_free(path);
    evaluator_free(evaluator);
}
END_TEST

START_TEST (null_path)
{
    document_model *bad_model = make_model(1);
    evaluator_context *evaluator = make_evaluator(bad_model, NULL);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_PATH_IS_NULL);
    model_free(bad_model);
    evaluator_free(evaluator);
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

    document_model *bad_model = make_model(1);
    evaluator_context *evaluator = make_evaluator(bad_model, path);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_NO_DOCUMENT_IN_MODEL);
    model_free(bad_model);
    path_free(path);
    evaluator_free(evaluator);
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

    document_model *bad_model = make_model(1);
    node *document = (node *)calloc(1, sizeof(struct node));
    assert_not_null(document);
    document->tag.kind = DOCUMENT;
    document->tag.name = NULL;
    document->tag.name_length = 0;
    document->content.size = 0;
    document->content.document.root = NULL;
    model_add(bad_model, document);

    evaluator_context *evaluator = make_evaluator(bad_model, path);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_NO_ROOT_IN_DOCUMENT);
    model_free(bad_model);
    path_free(path);
    evaluator_free(evaluator);
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

    document_model *bad_model = make_model(1);
    node *root = make_mapping_node(1);
    node *document = make_document_node(root);
    model_add(bad_model, document);

    evaluator_context *evaluator = make_evaluator(bad_model, path);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_PATH_IS_NOT_ABSOLUTE);
    model_free(bad_model);
    path_free(path);
    evaluator_free(evaluator);
}
END_TEST


START_TEST (empty_path)
{
    jsonpath *path = (jsonpath *)calloc(1, sizeof(jsonpath));
    assert_not_null(path);
    path->kind = ABSOLUTE_PATH;
    path->length = 0;
    path->steps = NULL;

    document_model *bad_model = make_model(1);
    node *root = make_mapping_node(1);
    node *document = make_document_node(root);
    model_add(bad_model, document);

    evaluator_context *evaluator = make_evaluator(bad_model, path);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_PATH_IS_EMPTY);
    model_free(bad_model);
    free(path);
    evaluator_free(evaluator);
}
END_TEST

START_TEST (null_context)
{
    reset_errno();
    assert_null(evaluate(NULL));
    assert_errno(EINVAL);    
}
END_TEST

START_TEST (null_context_model)
{
    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = NULL;
    evaluator->path = NULL;
    evaluator->list = NULL;

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    free(evaluator);
}
END_TEST

START_TEST (null_context_path)
{
    document_model *bad_model = make_model(1);
    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = NULL;
    evaluator->list = NULL;

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    free(evaluator);
}
END_TEST

START_TEST (null_context_list)
{
    document_model *bad_model = make_model(1);
    jsonpath *path = (jsonpath *)calloc(1, sizeof(jsonpath));

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = path;
    evaluator->list = NULL;

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    free(path);
    free(evaluator);
}
END_TEST

START_TEST (null_context_document)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    document_model *bad_model = make_model(1);
    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = path;
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    path_free(path);
    free(evaluator);
}
END_TEST

START_TEST (null_context_document_root)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    node *document = (node *)calloc(1, sizeof(struct node));
    assert_not_null(document);
    document->tag.kind = DOCUMENT;
    document->tag.name = NULL;
    document->tag.name_length = 0;
    document->content.size = 0;
    document->content.document.root = NULL;

    document_model *bad_model = make_model(1);
    model_add(bad_model, document);

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = path;
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    nodelist_free(evaluator->list);
    path_free(path);
    free(evaluator);
}
END_TEST

START_TEST (relative_context_path)
{
    char *expression = "foo";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    document_model *bad_model = make_model(1);
    node *root = make_mapping_node(1);
    node *document = make_document_node(root);
    model_add(bad_model, document);

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = path;
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    nodelist_free(evaluator->list);
    path_free(path);
    free(evaluator);
}
END_TEST

START_TEST (empty_context_path)
{
    jsonpath *path = (jsonpath *)calloc(1, sizeof(jsonpath));
    assert_not_null(path);
    path->kind = ABSOLUTE_PATH;
    path->length = 0;
    path->steps = NULL;

    document_model *bad_model = make_model(1);
    node *root = make_mapping_node(1);
    node *document = make_document_node(root);
    model_add(bad_model, document);

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = path;
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    nodelist_free(evaluator->list);
    free(path);
    free(evaluator);
}
END_TEST

void evaluator_setup(void)
{
    model = make_model(1);
    assert_not_null(model);
    
    FILE *input = fopen("inventory.json", "r");
    assert_not_null(input);
    
    loader_context *loader = make_file_loader(input);    
    assert_not_null(loader);
    model = load(loader);
    assert_not_null(model);
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));

    int closed = fclose(input);
    assert_int_eq(0, closed);
    loader_free(loader);
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

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 1);
    assert_node_kind(nodelist_get(list, 0), MAPPING);
    assert_node_size(nodelist_get(list, 0), 1);
    assert_mapping_has_key(nodelist_get(list, 0), "store");

    nodelist_free(list);
}
END_TEST

START_TEST (single_name_step)
{
    char *expression = "$.store";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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
    char *expression = "$..author";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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
    char * expression = "$.store..price";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 6);

    assert_scalar_value(nodelist_get(list, 0), "8.95");
    assert_scalar_value(nodelist_get(list, 1), "12.99");
    assert_scalar_value(nodelist_get(list, 2), "8.99");
    assert_scalar_value(nodelist_get(list, 3), "22.99");
    assert_scalar_value(nodelist_get(list, 4), "13.29");
    assert_scalar_value(nodelist_get(list, 5), "19.95");
    
    nodelist_free(list);
}
END_TEST

START_TEST (long_path)
{
    char *expression = "$.store.bicycle.color";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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
    char *expression = "$.store.*";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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
    char *expression = "$..*";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 34);

    assert_node_kind(nodelist_get(list, 0), MAPPING);

    nodelist_free(list);
}
END_TEST

START_TEST (object_test)
{
    char *expression = "$.store.object()";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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
    char * expression = "$.store.book.array()";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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
    char * expression = "$.store.book[*].price.number()";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 5);

    assert_scalar_value(nodelist_get(list, 0), "8.95");
    assert_scalar_value(nodelist_get(list, 1), "12.99");
    assert_scalar_value(nodelist_get(list, 2), "8.99");
    assert_scalar_value(nodelist_get(list, 3), "22.99");
    
    nodelist_free(list);
}
END_TEST

bool scalar_true(node *each, void *context __attribute__((unused)))
{

    return SCALAR == node_get_kind(each) &&
        SCALAR_BOOLEAN == scalar_get_kind(each) &&
        scalar_boolean_is_true(each);
}

START_TEST (wildcard_predicate)
{
    char *expression = "$.store.book[*].author";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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
    char *expression = "$.store.bicycle[*].color";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate_on_scalar)
{
    char *expression = "$.store.bicycle.color[*]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.store.book[2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_subscript_predicate)
{
    char *expression = "$..book[2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate)
{
    char *expression = "$.store.book[:2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_slice_predicate)
{
    char *expression = "$..book[:2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_with_step)
{
    char *expression = "$.store.book[:2:2]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

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

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_negative_from)
{
    char *expression = "$.store.book[-1:]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get_value(book, "author");
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "夏目漱石 (NATSUME Sōseki)");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_slice_predicate_negative_from)
{
    char *expression = "$..book[-1:]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get_value(book, "author");
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "夏目漱石 (NATSUME Sōseki)");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_copy)
{
    char *expression = "$.store.book[::]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 5);

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

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_reverse)
{
    char *expression = "$.store.book[::-1]";
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    assert_not_null(parser);

    jsonpath *path = parse(parser);
    assert_not_null(path);
    assert_int_eq(JSONPATH_SUCCESS, parser_status(parser));
    parser_free(parser);

    evaluator_context *evaluator = make_evaluator(model, path);
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    path_free(path);

    assert_nodelist_length(list, 5);

    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 0), "author"), "夏目漱石 (NATSUME Sōseki)");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 1), "author"), "J. R. R. Tolkien");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 2), "author"), "Herman Melville");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 3), "author"), "Evelyn Waugh");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get_value(nodelist_get(list, 4), "author"), "Nigel Rees");
    assert_noerr();

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
    tcase_add_test(bad_input_case, null_context);
    tcase_add_test(bad_input_case, null_context_model);
    tcase_add_test(bad_input_case, null_context_path);
    tcase_add_test(bad_input_case, null_context_list);
    tcase_add_test(bad_input_case, null_context_document);
    tcase_add_test(bad_input_case, null_context_document_root);
    tcase_add_test(bad_input_case, relative_context_path);
    tcase_add_test(bad_input_case, empty_context_path);

    TCase *basic_case = tcase_create("basic");
    tcase_add_unchecked_fixture(basic_case, evaluator_setup, evaluator_teardown);
    tcase_add_test(basic_case, dollar_only);
    tcase_add_test(basic_case, single_name_step);
    tcase_add_test(basic_case, long_path);
    tcase_add_test(basic_case, wildcard);
    tcase_add_test(basic_case, object_test);
    tcase_add_test(basic_case, array_test);
    tcase_add_test(basic_case, number_test);

    TCase *predicate_case = tcase_create("predicate");
    tcase_add_unchecked_fixture(predicate_case, evaluator_setup, evaluator_teardown);
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
    tcase_add_unchecked_fixture(recursive_case, evaluator_setup, evaluator_teardown);
    tcase_add_test(recursive_case, simple_recursive_step);
    tcase_add_test(recursive_case, compound_recursive_step);
    tcase_add_test(recursive_case, recursive_slice_predicate);
    tcase_add_test(recursive_case, recursive_subscript_predicate);
    tcase_add_test(recursive_case, recursive_slice_predicate_negative_from);
    tcase_add_test(recursive_case, recursive_wildcard);
    
    Suite *evaluator = suite_create("Evaluator");
    suite_add_tcase(evaluator, bad_input_case);
    suite_add_tcase(evaluator, basic_case);
    suite_add_tcase(evaluator, predicate_case);
    suite_add_tcase(evaluator, recursive_case);

    return evaluator;
}

