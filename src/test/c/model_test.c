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

void model_setup(void);
void model_teardown(void);

bool check_sequence(node *each, void *context);
bool check_mapping(node *key, node *value, void *context);
bool fail_sequence(node *each, void *context);
bool fail_mapping(node *key, node *value, void *context);

/**
  The document that will be used for testing this module is as follows:

  one:
    - "foo1"
    - 1.5
  two: "foo2"
  three: false
  four: true
 */
static document_model model;

START_TEST (null_model)
{
    errno = 0;
    assert_null(model_get_document(NULL, 0));
    assert_int_eq(EINVAL, errno);

    errno = 0;
    assert_null(model_get_document_root(NULL, 0));
    assert_int_eq(EINVAL, errno);

    errno = 0;
    assert_int_eq(0, model_get_document_count(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_node)
{
    errno = 0;
    assert_int_eq(-1, node_get_kind(NULL));
    assert_int_eq(EINVAL, errno);

    errno = 0;
    assert_null(node_get_name(NULL));
    assert_int_eq(EINVAL, errno);

    errno = 0;
    assert_int_eq(0, node_get_name_length(NULL));
    assert_int_eq(EINVAL, errno);

    errno = 0;
    assert_int_eq(0, node_get_size(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_document)
{
    errno = 0;
    assert_null(document_get_root(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_scalar)
{
    errno = 0;
    assert_null(scalar_get_value(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_sequence)
{
    errno = 0;
    assert_null(sequence_get(NULL, 0));
    assert_int_eq(EINVAL, errno);

    errno = 0;
    assert_null(sequence_get_all(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (null_mapping)
{
    errno = 0;
    assert_null(mapping_get_value(NULL, NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(mapping_get_value_scalar_key(NULL, NULL, 0));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(mapping_get_value_node_key(NULL, NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_false(mapping_contains_key(NULL, NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_false(mapping_contains_node_key(NULL, NULL));
    assert_int_eq(EINVAL, errno);
    errno = 0;
    assert_null(mapping_get_all(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

void model_setup(void)
{
    assert_true(model_init(&model, 1));

    errno = 0;
    node *root = make_mapping_node(4);
    assert_int_eq(0, errno);
    assert_not_null(root);

    errno = 0;
    node *foo1 = make_scalar_node((uint8_t *)"foo1", 4, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(foo1);
    errno = 0;
    node *one_point_five = make_scalar_node((uint8_t *)"1.5", 4, SCALAR_NUMBER);
    assert_int_eq(0, errno);
    assert_not_null(one_point_five);
    errno = 0;
    node *one_value = make_sequence_node(2);
    assert_int_eq(0, errno);
    assert_not_null(one_value);
    errno = 0;
    sequence_add(one_value, foo1);
    assert_int_eq(0, errno);
    errno = 0;
    sequence_add(one_value, one_point_five);
    assert_int_eq(0, errno);
    errno = 0;
    node *one = make_scalar_node((uint8_t *)"one", 3, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(one);
    errno = 0;
    mapping_put(root, one, one_value);
    assert_int_eq(0, errno);
    
    errno = 0;
    node *two = make_scalar_node((uint8_t *)"two", 3, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(two);
    errno = 0;
    node *two_value = make_scalar_node((uint8_t *)"foo2", 4, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(two_value);
    errno = 0;
    mapping_put(root, two, two_value);
    assert_int_eq(0, errno);
    
    errno = 0;
    node *three = make_scalar_node((uint8_t *)"three", 5, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(three);
    errno = 0;
    node *three_value = make_scalar_node((uint8_t *)"false", 5, SCALAR_BOOLEAN);
    assert_int_eq(0, errno);
    assert_not_null(three_value);
    errno = 0;
    mapping_put(root, three, three_value);
    assert_int_eq(0, errno);

    errno = 0;
    node *four = make_scalar_node((uint8_t *)"four", 4, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(four);
    errno = 0;
    node *four_value = make_scalar_node((uint8_t *)"true", 4, SCALAR_BOOLEAN);
    assert_int_eq(0, errno);
    assert_not_null(four_value);
    errno = 0;
    mapping_put(root, four, four_value);
    assert_int_eq(0, errno);

    errno = 0;
    node *document = make_document_node(root);
    assert_int_eq(0, errno);
    assert_not_null(document);
    errno = 0;
    model_add(&model, document);
    assert_int_eq(0, errno);
}

void model_teardown(void)
{
    model_free(&model);
    assert_null(model.documents);
    assert_int_eq(0, model.size);
}

START_TEST (constructors)
{
    errno = 0;
    node *s = make_scalar_node(NULL, 0, SCALAR_STRING);
    assert_int_eq(EINVAL, errno);
    assert_null(s);
    errno = 0;
    s = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(s);
    
    errno = 0;
    node *d = make_document_node(NULL);
    assert_int_eq(EINVAL, errno);
    assert_null(d);
    d = make_document_node(s);
    assert_int_eq(0, errno);
    assert_not_null(d);
    node_free(d); // N.B. - this will also free `s'

    errno = 0;
    node *v = make_sequence_node(0);
    assert_int_eq(EINVAL, errno);
    assert_null(v);
    errno = 0;
    v = make_sequence_node(1);
    assert_int_eq(0, errno);
    assert_not_null(v);
    assert_not_null(v->content.sequence.value);
    assert_int_eq(1, v->content.sequence.capacity);
    assert_int_eq(0, v->content.size);
    node_free(v);
    
    errno = 0;
    node *m = make_mapping_node(0);
    assert_int_eq(EINVAL, errno);
    assert_null(m);
    errno = 0;
    m = make_mapping_node(1);
    assert_int_eq(0, errno);
    assert_not_null(m);
    assert_not_null(m->content.mapping.value);
    assert_int_eq(1, m->content.mapping.capacity);
    assert_int_eq(0, m->content.size);
    node_free(m);
}
END_TEST

START_TEST (document)
{
    errno = 0;
    size_t c = model_get_document_count(&model);
    assert_int_eq(0, errno);
    assert_int_eq(1, c);
    
    errno = 0;
    node *d = model_get_document(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(d);
    
    errno = 0;
    node *bogus = model_get_document(&model, 1);
    assert_int_eq(EINVAL, errno);
    assert_null(bogus);
    
    errno = 0;
    node *r1 = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r1);
    assert_int_eq(MAPPING, node_get_kind(r1));

    errno = 0;
    node *r2 = document_get_root(d);
    assert_int_eq(0, errno);
    assert_not_null(r2);
    assert_ptr_eq(r1, r2);
}
END_TEST

START_TEST (nodes)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));

    errno = 0;
    unsigned char *n = node_get_name(r);
    assert_int_eq(0, errno);
    assert_null(n);

    errno = 0;
    size_t l = node_get_name_length(r);
    assert_int_eq(0, errno);
    assert_int_eq(0, l);

    errno = 0;
    size_t s = node_get_size(r);
    assert_int_eq(0, errno);
    assert_int_eq(4, s);
}
END_TEST

START_TEST (scalar)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));

    errno = 0;
    node *s = mapping_get_value(r, "two");
    assert_int_eq(0, errno);
    assert_not_null(s);
    assert_int_eq(SCALAR, node_get_kind(s));
    assert_buf_eq("foo2", 4, scalar_get_value(s), node_get_size(s));
}
END_TEST

START_TEST (scalar_boolean)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));

    errno = 0;
    node *three = mapping_get_value(r, "three");
    assert_int_eq(0, errno);
    assert_not_null(three);
    assert_int_eq(SCALAR, node_get_kind(three));
    assert_true(scalar_boolean_is_false(three));
    assert_false(scalar_boolean_is_true(three));

    errno = 0;
    node *four = mapping_get_value(r, "four");
    assert_int_eq(0, errno);
    assert_not_null(four);
    assert_int_eq(SCALAR, node_get_kind(four));
    assert_true(scalar_boolean_is_true(four));
    assert_false(scalar_boolean_is_false(four));
}
END_TEST

START_TEST (sequence)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));

    errno = 0;
    node *s = mapping_get_value(r, "one");
    assert_int_eq(0, errno);
    assert_not_null(s);
    assert_int_eq(SEQUENCE, node_get_kind(s));
    assert_int_eq(2, node_get_size(s));
    
    errno = 0;
    node *zero = sequence_get(s, 0);
    assert_int_eq(0, errno);
    assert_not_null(zero);
    assert_int_eq(SCALAR, node_get_kind(zero));
    
    errno = 0;
    node *one = sequence_get(s, 1);
    assert_int_eq(0, errno);
    assert_not_null(one);
    assert_int_eq(SCALAR, node_get_kind(one));

    errno = 0;
    node **all = sequence_get_all(s);
    assert_int_eq(0, errno);
    assert_not_null(all);
    assert_ptr_eq(zero, all[0]);
    assert_ptr_eq(one, all[1]);

    errno = 0;
    node *x = make_scalar_node((uint8_t *)"x", 1, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(x);
    errno = 0;
    node *y = make_scalar_node((uint8_t *)"y", 1, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(y);
    errno = 0;
    node *z = make_scalar_node((uint8_t *)"z", 1, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_not_null(z);
    errno = 0;
    node *xyz = make_sequence_node(2);
    assert_int_eq(0, errno);
    assert_not_null(xyz);
    assert_int_eq(0, xyz->content.size);
    assert_int_eq(2, xyz->content.sequence.capacity);

    errno = 0;
    sequence_add(xyz, x);
    assert_int_eq(0, errno);
    errno = 0;
    sequence_add(xyz, y);
    assert_int_eq(0, errno);
    errno = 0;
    sequence_add(xyz, z);
    assert_int_eq(0, errno);
    assert_int_eq(3, xyz->content.size);
    assert_int_eq(5, xyz->content.sequence.capacity);
    assert_ptr_eq(x, sequence_get(xyz, 0));
    assert_ptr_eq(y, sequence_get(xyz, 1));
    assert_ptr_eq(z, sequence_get(xyz, 2));
    node_free(xyz);
}
END_TEST

START_TEST (mapping)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));
    assert_int_eq(4, node_get_size(r));

    assert_true(mapping_contains_key(r, "two"));
    assert_false(mapping_contains_key(r, "bogus"));

    errno = 0;
    node *key = make_scalar_node((uint8_t *)"two", 3, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_true(mapping_contains_node_key(r, key));

    errno = 0;
    node *bogus_key = make_scalar_node((uint8_t *)"bogus", 5, SCALAR_STRING);
    assert_int_eq(0, errno);
    assert_false(mapping_contains_node_key(r, bogus_key));
    assert_null(mapping_get_value(r, "bogus"));
    assert_null(mapping_get_value_scalar_key(r, (uint8_t *)"bogus", 5));
    assert_null(mapping_get_value_node_key(r, bogus_key));

    errno = 0;
    node *string_value = mapping_get_value(r, "two");
    assert_int_eq(0, errno);
    assert_not_null(string_value);
    assert_int_eq(SCALAR, node_get_kind(string_value));

    errno = 0;
    node *scalar_value = mapping_get_value_scalar_key(r, (uint8_t *)"two", 3);
    assert_int_eq(0, errno);
    assert_not_null(scalar_value);
    assert_int_eq(SCALAR, node_get_kind(scalar_value));

    errno = 0;
    node *node_value = mapping_get_value_node_key(r, key);
    assert_int_eq(0, errno);
    assert_not_null(node_value);
    assert_int_eq(SCALAR, node_get_kind(node_value));

    assert_ptr_eq(string_value, scalar_value);
    assert_ptr_eq(string_value, node_value);
    assert_ptr_eq(scalar_value, node_value);

    errno = 0;
    key_value_pair **all = mapping_get_all(r);
    assert_int_eq(0, errno);
    assert_not_null(all[0]);
    assert_not_null(all[1]);
    assert_not_null(all[2]);
}
END_TEST

START_TEST (sequence_iteration)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));
    
    errno = 0;
    node *s = mapping_get_value(r, "one");
    assert_int_eq(0, errno);
    assert_not_null(s);
    assert_int_eq(SEQUENCE, node_get_kind(s));
    assert_int_eq(2, node_get_size(s));
    
    size_t count = 0;
    errno = 0;
    assert_true(iterate_sequence(s, check_sequence, &count));
    assert_int_eq(0, errno);
    assert_int_eq(2, count);
}
END_TEST

bool check_sequence(node *each, void *context)
{
    assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    return true;
}

START_TEST (fail_sequence_iteration)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));
    
    errno = 0;
    node *s = mapping_get_value(r, "one");
    assert_int_eq(0, errno);
    assert_not_null(s);
    assert_int_eq(SEQUENCE, node_get_kind(s));
    assert_int_eq(2, node_get_size(s));
    
    size_t count = 0;
    errno = 0;
    assert_false(iterate_sequence(s, fail_sequence, &count));
    assert_int_eq(0, errno);
    assert_int_eq(1, count);
}
END_TEST

bool fail_sequence(node *each, void *context)
{
#pragma unused(each)

    size_t *count = (size_t *)context;
    if(0 < *count)
    {
        return false;
    }
    else
    {
        (*count)++;
        return true;
    }
}

START_TEST (mapping_iteration)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));
    assert_int_eq(4, node_get_size(r));
    
    errno = 0;
    size_t count = 0;
    assert_true(iterate_mapping(r, check_mapping, &count));
    assert_int_eq(4, count);
}
END_TEST

bool check_mapping(node *key, node *value, void *context)
{
    assert_not_null(key);
    assert_not_null(value);
    size_t *count = (size_t *)context;
    (*count)++;
    return true;
}

START_TEST (fail_mapping_iteration)
{
    errno = 0;
    node *r = model_get_document_root(&model, 0);
    assert_int_eq(0, errno);
    assert_not_null(r);
    assert_int_eq(MAPPING, node_get_kind(r));
    assert_int_eq(4, node_get_size(r));
    
    size_t count = 0;
    errno = 0;
    assert_false(iterate_mapping(r, fail_mapping, &count));
    assert_int_eq(0, errno);
    assert_int_eq(2, count);
}
END_TEST

bool fail_mapping(node *key, node *value, void *context)
{
    assert_not_null(key);
    assert_not_null(value);
    size_t *count = (size_t *)context;
    if(1 < *count)
    {
        return false;
    }
    else
    {
        (*count)++;
        return true;
    }
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
    tcase_add_checked_fixture(basic, model_setup, model_teardown);
    tcase_add_test(basic, constructors);
    tcase_add_test(basic, document);
    tcase_add_test(basic, nodes);
    tcase_add_test(basic, scalar);
    tcase_add_test(basic, scalar_boolean);
    tcase_add_test(basic, sequence);
    tcase_add_test(basic, mapping);

    TCase *iteration = tcase_create("iteration");
    tcase_add_checked_fixture(iteration, model_setup, model_teardown);
    tcase_add_test(iteration, sequence_iteration);
    tcase_add_test(iteration, mapping_iteration);
    tcase_add_test(iteration, fail_sequence_iteration);
    tcase_add_test(iteration, fail_mapping_iteration);

    Suite *model_suite = suite_create("Model");
    suite_add_tcase(model_suite, bad_input);
    suite_add_tcase(model_suite, basic);
    suite_add_tcase(model_suite, iteration);
    
    return model_suite;
}
