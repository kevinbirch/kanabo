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

#include <check.h>

#include "model.h"
#include "loader.h"
#include "test.h"

static const unsigned char * const YAML = (unsigned char *)
    "one:\n"
    "  - foo1\n"
    "  - bar1\n"
    "\n"
    "two: foo2\n"
    "\n"
    "three: foo3\n";

static document_model model;

void setup(void);
void teardown(void);

START_TEST (null_model)
{
    node *d = model_get_document(NULL, 0);
    ck_assert_null(d);

    node *r = model_get_document_root(NULL, 0);
    ck_assert_null(r);

    size_t c = model_get_document_count(NULL);
    ck_assert_int_eq(0, c);
}
END_TEST

START_TEST (null_node)
{
    enum kind k = node_get_kind(NULL);
    ck_assert_int_eq(-1, k);

    unsigned char *n = node_get_name(NULL);
    ck_assert_null(n);

    size_t l = node_get_name_length(NULL);
    ck_assert_int_eq(0, l);

    size_t s = node_get_size(NULL);
    ck_assert_int_eq(0, s);
}
END_TEST

START_TEST (null_document)
{
    node *r = document_get_root(NULL);
    ck_assert_null(r);
}
END_TEST

START_TEST (null_scalar)
{
    unsigned char *s = scalar_get_value(NULL);
    ck_assert_null(s);
}
END_TEST

START_TEST (null_sequence)
{
    node *i = sequence_get_item(NULL, 0);
    ck_assert_null(i);

    node **a = sequence_get_all(NULL);
    ck_assert_null(a);
}
END_TEST

START_TEST (null_mapping)
{
    node *x = mapping_get_value(NULL, NULL);
    ck_assert_null(x);
    
    node *y = mapping_get_value_scalar_key(NULL, NULL, 0);
    ck_assert_null(y);
    
    node *z = mapping_get_value_node_key(NULL, NULL);
    ck_assert_null(z);
    
    bool b1 = mapping_contains_key(NULL, NULL);
    ck_assert_false(b1);

    bool b2 = mapping_contains_node_key(NULL, NULL);
    ck_assert_false(b2);

    key_value_pair **p = mapping_get_all(NULL);
    ck_assert_null(p);
}
END_TEST

START_TEST (constructors)
{
    node *d = make_document_node();
    ck_assert_not_null(d);
    
    node *s = make_scalar_node(NULL, 0);
    ck_assert_null(s);
    s = make_scalar_node((unsigned char *)"foo", 3);
    ck_assert_not_null(s);
    
    node *v = make_sequence_node();
    ck_assert_not_null(v);
    
    node *m = make_mapping_node();
    ck_assert_not_null(m);    
}
END_TEST

void setup(void)
{
    size_t yaml_size = strlen((char *)YAML);
    int result = build_model_from_string(YAML, yaml_size, &model);

    ck_assert_int_eq(0, result);
    ck_assert_not_null(model.documents);
    ck_assert_int_eq(1, model_get_document_count(&model));
}

void teardown(void)
{
    free_model(&model);
    ck_assert_null(model.documents);
    ck_assert_int_eq(0, model.size);
}

START_TEST (document)
{
    size_t c = model_get_document_count(&model);
    ck_assert_int_eq(1, c);
    
    node *d = model_get_document(&model, 0);
    ck_assert_not_null(d);
    
    node *bogus = model_get_document(&model, 1);
    ck_assert_null(bogus);
    
    node *r1 = model_get_document_root(&model, 0);
    ck_assert_not_null(r1);
    ck_assert_int_eq(MAPPING, node_get_kind(r1));

    node *r2 = document_get_root(d);
    ck_assert_not_null(r2);
    ck_assert_int_eq(r1, r2);
}
END_TEST

START_TEST (nodes)
{
    node *r = model_get_document_root(&model, 0);
    ck_assert_not_null(r);
    ck_assert_int_eq(MAPPING, node_get_kind(r));

    unsigned char *n = node_get_name(r);
    ck_assert_null(n);

    size_t l = node_get_name_length(r);
    ck_assert_int_eq(0, l);

    size_t s = node_get_size(r);
    ck_assert_int_eq(3, s);
}
END_TEST

START_TEST (scalar)
{
    node *r = model_get_document_root(&model, 0);
    ck_assert_not_null(r);
    ck_assert_int_eq(MAPPING, node_get_kind(r));

    node *s = mapping_get_value(r, "two");
    ck_assert_not_null(s);
    ck_assert_int_eq(SCALAR, node_get_kind(s));
    ck_assert_buf_eq("foo2", 4, scalar_get_value(s), node_get_size(s));
}
END_TEST

START_TEST (sequence)
{
    node *r = model_get_document_root(&model, 0);
    ck_assert_not_null(r);
    ck_assert_int_eq(MAPPING, node_get_kind(r));

    node *s = mapping_get_value(r, "one");
    ck_assert_not_null(s);
    ck_assert_int_eq(SEQUENCE, node_get_kind(s));
    ck_assert_int_eq(2, node_get_size(s));
    
    node *zero = sequence_get_item(s, 0);
    ck_assert_not_null(zero);
    ck_assert_int_eq(SCALAR, node_get_kind(zero));
    
    node *one = sequence_get_item(s, 1);
    ck_assert_not_null(one);
    ck_assert_int_eq(SCALAR, node_get_kind(one));

    node **all = sequence_get_all(s);
    ck_assert_not_null(all);
    ck_assert_int_eq(zero, all[0]);
    ck_assert_int_eq(one, all[1]);
}
END_TEST

START_TEST (mapping)
{
    node *r = model_get_document_root(&model, 0);
    ck_assert_not_null(r);
    ck_assert_int_eq(MAPPING, node_get_kind(r));
    ck_assert_int_eq(3, node_get_size(r));

    ck_assert_true(mapping_contains_key(r, "two"));
    ck_assert_false(mapping_contains_key(r, "bogus"));

    node *key = make_scalar_node((unsigned char *)"two", 3);
    ck_assert_true(mapping_contains_node_key(r, key));

    node *bogus_key = make_scalar_node((unsigned char *)"bogus", 5);
    ck_assert_false(mapping_contains_node_key(r, bogus_key));
    ck_assert_null(mapping_get_value(r, "bogus"));
    ck_assert_null(mapping_get_value_scalar_key(r, (unsigned char *)"bogus", 5));
    ck_assert_null(mapping_get_value_node_key(r, bogus_key));

    node *string_value = mapping_get_value(r, "two");
    ck_assert_not_null(string_value);
    ck_assert_int_eq(SCALAR, node_get_kind(string_value));

    node *scalar_value = mapping_get_value_scalar_key(r, (unsigned char *)"two", 3);
    ck_assert_not_null(scalar_value);
    ck_assert_int_eq(SCALAR, node_get_kind(scalar_value));

    node *node_value = mapping_get_value_node_key(r, key);
    ck_assert_not_null(node_value);
    ck_assert_int_eq(SCALAR, node_get_kind(node_value));

    ck_assert_int_eq(string_value, scalar_value);
    ck_assert_int_eq(string_value, node_value);
    ck_assert_int_eq(scalar_value, node_value);

    key_value_pair **all = mapping_get_all(r);
    ck_assert_not_null(all[0]);
    ck_assert_not_null(all[1]);
    ck_assert_not_null(all[2]);
}
END_TEST

void check_sequence(node *each, void *context);
void check_mapping(node *key, node *value, void *context);

START_TEST (sequence_iteration)
{
    node *r = model_get_document_root(&model, 0);
    ck_assert_not_null(r);
    ck_assert_int_eq(MAPPING, node_get_kind(r));
    
    node *s = mapping_get_value(r, "one");
    ck_assert_not_null(s);
    ck_assert_int_eq(SEQUENCE, node_get_kind(s));
    ck_assert_int_eq(2, node_get_size(s));
    
    size_t count = 0;
    iterate_sequence(s, check_sequence, &count);
    ck_assert_int_eq(2, count);
}
END_TEST

void check_sequence(node *each, void *context)
{
    ck_assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
}

START_TEST (mapping_iteration)
{
    node *r = model_get_document_root(&model, 0);
    ck_assert_not_null(r);
    ck_assert_int_eq(MAPPING, node_get_kind(r));
    ck_assert_int_eq(3, node_get_size(r));
    
    size_t count = 0;
    iterate_mapping(r, check_mapping, &count);
    ck_assert_int_eq(3, count);
}
END_TEST

void check_mapping(node *key, node *value, void *context)
{
    ck_assert_not_null(key);
    ck_assert_not_null(value);
    size_t *count = (size_t *)context;
    (*count)++;
}

Suite *model_suite(void)
{
    TCase *bad_input = tcase_create("bad input");
    tcase_add_test(bad_input, null_model);
    tcase_add_test(bad_input, null_node);
    tcase_add_test(bad_input, null_document);
    tcase_add_test(bad_input, null_scalar);
    tcase_add_test(bad_input, null_sequence);
    tcase_add_test(bad_input, null_mapping);

    TCase *basic = tcase_create("basic");
    tcase_add_checked_fixture(basic, setup, teardown);
    tcase_add_test(basic, constructors);
    tcase_add_test(basic, document);
    tcase_add_test(basic, nodes);
    tcase_add_test(basic, scalar);
    tcase_add_test(basic, sequence);
    tcase_add_test(basic, mapping);

    TCase *iteration = tcase_create("iteration");
    tcase_add_checked_fixture(iteration, setup, teardown);
    tcase_add_test(iteration, sequence_iteration);
    tcase_add_test(iteration, mapping_iteration);

    Suite *model_suite = suite_create("Model_Suite");
    suite_add_tcase(model_suite, bad_input);
    suite_add_tcase(model_suite, basic);
    suite_add_tcase(model_suite, iteration);
    
    return model_suite;
}
