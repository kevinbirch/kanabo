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

#include <check.h>

#include "loader.h"
#include "test.h"

static const unsigned char * const YAML = (unsigned char *)
    "one:\n"
    "  - foo1\n"
    "  - bar1\n"
    "\n"
    "two: foo2\n"
    "\n"
    "three: foo3\n"
    "\n"
    "four:\n"
    "  - foo4\n"
    "  - bar4\n"
    "\n"
    "five: foo5\n";

static void assert_model_state(int result, document_model *model);

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
    int result = build_model_from_file(input, &model);

    assert_model_state(result, &model);

    fclose(input);
}
END_TEST

START_TEST (load_from_string)
{
    document_model model;
    size_t yaml_size = strlen((char *)YAML);
    int result = build_model_from_string(YAML, yaml_size, &model);

    assert_model_state(result, &model);
}
END_TEST

static void assert_model_state(int result, document_model *model)
{
    ck_assert_int_eq(0, result);
    ck_assert_not_null(model);
    ck_assert_int_eq(1, model_get_document_count(model));

    node *root = model_get_document_root(model, 0);
    ck_assert_not_null(root);
    
    ck_assert_int_eq(MAPPING, node_get_kind(root));
    ck_assert_int_eq(5, node_get_size(root));
    ck_assert_not_null(mapping_get_all(root));

    key_value_pair *one = mapping_get_key_value(root, 0);
    ck_assert_not_null(one);
    ck_assert_int_eq(SCALAR, node_get_kind(one->key));
    ck_assert_buf_eq("one", 3, scalar_get_value(one->key), node_get_size(one->key));
    ck_assert_int_eq(SEQUENCE, node_get_kind(one->value));
    ck_assert_int_eq(2, node_get_size(one->value));
    node *s0_0 = sequence_get_item(one->value, 0);
    ck_assert_int_eq(SCALAR, node_get_kind(s0_0));
    ck_assert_buf_eq("foo1", 4, scalar_get_value(s0_0), node_get_size(s0_0));
    node *s0_1 = sequence_get_item(one->value, 1);
    ck_assert_int_eq(SCALAR, node_get_kind(s0_1));
    ck_assert_buf_eq("bar1", 4, scalar_get_value(s0_1), node_get_size(s0_1));

    key_value_pair *two = mapping_get_key_value(root, 1);
    ck_assert_not_null(two);
    ck_assert_int_eq(SCALAR, node_get_kind(two->key));
    ck_assert_buf_eq("two", 3, scalar_get_value(two->key), node_get_size(two->key));
    ck_assert_int_eq(SCALAR, node_get_kind(two->value));
    ck_assert_buf_eq("foo2", 4, scalar_get_value(two->value), node_get_size(two->value));

    key_value_pair *three = mapping_get_key_value(root, 2);
    ck_assert_not_null(three);
    ck_assert_int_eq(SCALAR, node_get_kind(three->key));
    ck_assert_buf_eq("three", 5, scalar_get_value(three->key), node_get_size(three->key));
    ck_assert_int_eq(SCALAR, node_get_kind(three->value));
    ck_assert_buf_eq("foo3", 4, scalar_get_value(three->value), node_get_size(three->value));

    key_value_pair *four = mapping_get_key_value(root, 3);
    ck_assert_not_null(four);
    ck_assert_int_eq(SCALAR, node_get_kind(four->key));
    ck_assert_buf_eq("four", 4, scalar_get_value(four->key), node_get_size(four->key));
    ck_assert_int_eq(SEQUENCE, node_get_kind(four->value));
    node *s3_0 = sequence_get_item(four->value, 0);
    ck_assert_int_eq(SCALAR, node_get_kind(s3_0));
    ck_assert_buf_eq("foo4", 4, scalar_get_value(s3_0), node_get_size(s3_0));
    node *s3_1 = sequence_get_item(four->value, 1);
    ck_assert_int_eq(SCALAR, node_get_kind(s3_0));
    ck_assert_buf_eq("bar4", 4, scalar_get_value(s3_1), node_get_size(s3_1));

    key_value_pair *five = mapping_get_key_value(root, 4);
    ck_assert_not_null(five);
    ck_assert_int_eq(SCALAR, node_get_kind(five->key));
    ck_assert_buf_eq("five", 4, scalar_get_value(five->key), node_get_size(five->key));
    ck_assert_int_eq(SCALAR, node_get_kind(five->value));
    ck_assert_buf_eq("foo5", 4, scalar_get_value(five->value), node_get_size(five->value));
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

