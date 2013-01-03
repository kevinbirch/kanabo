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

#include "model.h"
#include "test.h"

/* 
  The document that will be used for testing this module is as follows:

  one:
    - foo1
    - bar1
  two: foo2
  three: foo3

 */

static document_model model;

void setup(void);
void teardown(void);

START_TEST (null_model)
{
    errno = 0;
    ck_assert_null(model_get_document(NULL, 0));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_null(model_get_document_root(NULL, 0));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_int_eq(0, model_get_document_count(NULL));
    ck_assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_node)
{
    errno = 0;
    ck_assert_int_eq(-1, node_get_kind(NULL));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_null(node_get_name(NULL));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_int_eq(0, node_get_name_length(NULL));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_int_eq(0, node_get_size(NULL));
    ck_assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_document)
{
    errno = 0;
    ck_assert_null(document_get_root(NULL));
    ck_assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_scalar)
{
    errno = 0;
    ck_assert_null(scalar_get_value(NULL));
    ck_assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_sequence)
{
    errno = 0;
    ck_assert_null(sequence_get_item(NULL, 0));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_null(sequence_get_all(NULL));
    ck_assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_mapping)
{
    errno = 0;
    ck_assert_null(mapping_get_value(NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(mapping_get_value_scalar_key(NULL, NULL, 0));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(mapping_get_value_node_key(NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_false(mapping_contains_key(NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_false(mapping_contains_node_key(NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    errno = 0;
    ck_assert_null(mapping_get_all(NULL));
    ck_assert_int_eq(EINVAL, errno);
}
END_TEST

void setup(void)
{
    ck_assert_true(init_model(&model, 1));

    node *root = make_mapping_node(3);
    ck_assert_not_null(root);

    node *foo1 = make_scalar_node((unsigned char *)"foo1", 4);
    ck_assert_not_null(foo1);
    node *bar1 = make_scalar_node((unsigned char *)"bar1", 4);
    ck_assert_not_null(bar1);
    node *one_value = make_sequence_node(2);
    ck_assert_not_null(one_value);
    sequence_add(one_value, foo1);
    sequence_add(one_value, bar1);
    node *one = make_scalar_node((unsigned char *)"one", 3);
    ck_assert_not_null(one);
    mapping_put(root, one, one_value);
    
    node *two = make_scalar_node((unsigned char *)"two", 3);
    ck_assert_not_null(two);
    node *two_value = make_scalar_node((unsigned char *)"foo2", 4);
    ck_assert_not_null(two_value);
    mapping_put(root, two, two_value);
    
    node *three = make_scalar_node((unsigned char *)"three", 4);
    ck_assert_not_null(three);
    node *three_value = make_scalar_node((unsigned char *)"foo3", 4);
    ck_assert_not_null(three_value);
    mapping_put(root, three, three_value);

    node *document = make_document_node(root);
    ck_assert_not_null(document);
    model_add(&model, document);
}

void teardown(void)
{
    free_model(&model);
    ck_assert_null(model.documents);
    ck_assert_int_eq(0, model.size);
}

START_TEST (constructors)
{
    node *s = make_scalar_node(NULL, 0);
    ck_assert_null(s);
    s = make_scalar_node((unsigned char *)"foo", 3);
    ck_assert_not_null(s);
    
    node *d = make_document_node(NULL);
    ck_assert_null(d);
    d = make_document_node(s);
    ck_assert_not_null(d);
    free_node(d); // N.B. - this will also free `s'

    node *v = make_sequence_node(0);
    ck_assert_null(v);
    v = make_sequence_node(1);
    ck_assert_not_null(v);
    ck_assert_not_null(v->content.sequence.value);
    ck_assert_int_eq(1, v->content.sequence.capacity);
    ck_assert_int_eq(0, v->content.size);
    free_node(v);
    
    node *m = make_mapping_node(0);
    ck_assert_null(m);
    m = make_mapping_node(1);
    ck_assert_not_null(m);
    ck_assert_not_null(m->content.mapping.value);
    ck_assert_int_eq(1, m->content.mapping.capacity);
    ck_assert_int_eq(0, m->content.size);
    free_node(m);
}
END_TEST

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

    node *x = make_scalar_node((unsigned char *)"x", 1);
    ck_assert_not_null(x);
    node *y = make_scalar_node((unsigned char *)"y", 1);
    ck_assert_not_null(y);
    node *z = make_scalar_node((unsigned char *)"z", 1);
    ck_assert_not_null(z);
    node *xyz = make_sequence_node(2);
    ck_assert_not_null(xyz);
    ck_assert_int_eq(0, xyz->content.size);
    ck_assert_int_eq(2, xyz->content.sequence.capacity);

    sequence_add(xyz, x);
    sequence_add(xyz, y);
    sequence_add(xyz, z);
    ck_assert_int_eq(3, xyz->content.size);
    ck_assert_int_eq(5, xyz->content.sequence.capacity);
    ck_assert_int_eq(x, sequence_get_item(xyz, 0));
    ck_assert_int_eq(y, sequence_get_item(xyz, 1));
    ck_assert_int_eq(z, sequence_get_item(xyz, 2));
    free_node(xyz);
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
