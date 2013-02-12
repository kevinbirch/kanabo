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
#include <string.h>
#include <errno.h>

#include <check.h>

#include "loader.h"
#include "test.h"

static const unsigned char * const YAML = (unsigned char *)
    "one:\n"
    "  - foo1\n"
    "  - bar1\n"
    "\n"
    "two: \"foo2\"\n"
    "\n"
    "three: foo3\n"
    "\n"
    "four:\n"
    "  - true\n"
    "  - false\n"
    "\n"
    "five: 1.5\n";

static void assert_model_state(loader_result *result, document_model *model);

START_TEST (load_from_file)
{
    size_t yaml_size = strlen((char *)YAML);

    FILE *input = tmpfile();
    size_t written = fwrite(YAML, sizeof(char), yaml_size, input);
    ck_assert_int_eq(written, yaml_size);
    int ret = fflush(input);
    ck_assert_int_eq(0, ret);

    rewind(input);

    document_model model;
    loader_result *result = load_model_from_file(input, &model);

    assert_model_state(result, &model);

    fclose(input);
    model_free(&model);
    free_loader_result(result);
}
END_TEST

START_TEST (load_from_string)
{
    document_model model;
    size_t yaml_size = strlen((char *)YAML);
    loader_result *result = load_model_from_string(YAML, yaml_size, &model);

    assert_model_state(result, &model);
    model_free(&model);
    free(result);
}
END_TEST

static void assert_model_state(loader_result *result, document_model *model)
{
    ck_assert_int_eq(LOADER_SUCCESS, result->code);
    ck_assert_not_null(model);
    ck_assert_int_eq(1, model_get_document_count(model));

    node *root = model_get_document_root(model, 0);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(root);
    
    ck_assert_int_eq(MAPPING, node_get_kind(root));
    ck_assert_int_eq(5, node_get_size(root));
    ck_assert_not_null(mapping_get_all(root));

    node *one = mapping_get_value(root, "one");
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(one);
    ck_assert_int_eq(SEQUENCE, node_get_kind(one));
    ck_assert_int_eq(2, node_get_size(one));
    node *one_0 = sequence_get(one, 0);
    ck_assert_int_eq(0, errno);
    ck_assert_int_eq(SCALAR, node_get_kind(one_0));
    ck_assert_buf_eq("foo1", 4, scalar_get_value(one_0), node_get_size(one_0));
    ck_assert_int_eq(SCALAR_STRING, scalar_get_kind(one_0));
    node *one_1 = sequence_get(one, 1);
    ck_assert_int_eq(0, errno);
    ck_assert_int_eq(SCALAR, node_get_kind(one_1));
    ck_assert_buf_eq("bar1", 4, scalar_get_value(one_1), node_get_size(one_1));
    ck_assert_int_eq(SCALAR_STRING, scalar_get_kind(one_1));

    node *two = mapping_get_value(root, "two");
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(two);
    ck_assert_int_eq(SCALAR, node_get_kind(two));
    ck_assert_buf_eq("foo2", 4, scalar_get_value(two), node_get_size(two));
    ck_assert_int_eq(SCALAR_STRING, scalar_get_kind(two));

    node *three = mapping_get_value(root, "three");
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(three);
    ck_assert_int_eq(SCALAR, node_get_kind(three));
    ck_assert_buf_eq("foo3", 4, scalar_get_value(three), node_get_size(three));
    ck_assert_int_eq(SCALAR_STRING, scalar_get_kind(three));

    node *four = mapping_get_value(root, "four");
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(four);
    ck_assert_int_eq(SEQUENCE, node_get_kind(four));
    node *four_0 = sequence_get(four, 0);
    ck_assert_int_eq(0, errno);
    ck_assert_int_eq(SCALAR, node_get_kind(four_0));
    ck_assert_buf_eq("true", 4, scalar_get_value(four_0), node_get_size(four_0));
    ck_assert_int_eq(SCALAR_BOOLEAN, scalar_get_kind(four_0));
    ck_assert_true(scalar_boolean_is_true(four_0));
    ck_assert_false(scalar_boolean_is_false(four_0));
    node *four_1 = sequence_get(four, 1);
    ck_assert_int_eq(0, errno);
    ck_assert_int_eq(SCALAR, node_get_kind(four_0));
    ck_assert_buf_eq("false", 5, scalar_get_value(four_1), node_get_size(four_1));
    ck_assert_int_eq(SCALAR_BOOLEAN, scalar_get_kind(four_1));
    ck_assert_true(scalar_boolean_is_false(four_1));
    ck_assert_false(scalar_boolean_is_true(four_1));

    node *five = mapping_get_value(root, "five");
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(five);
    ck_assert_int_eq(SCALAR, node_get_kind(five));
    ck_assert_buf_eq("1.5", 3, scalar_get_value(five), node_get_size(five));
    ck_assert_int_eq(SCALAR_NUMBER, scalar_get_kind(five));
}

Suite *loader_suite(void)
{
    TCase *basic = tcase_create("basic");
    tcase_add_test(basic, load_from_file);
    tcase_add_test(basic, load_from_string);

    Suite *loader = suite_create("Loader");
    suite_add_tcase(loader, basic);
    
    return loader;
}

