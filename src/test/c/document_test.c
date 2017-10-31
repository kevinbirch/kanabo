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

#include "document.h"
#include "test.h"
#include "test_document.h"

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
static document_model *model;

START_TEST (null_model)
{
    reset_errno();
    assert_null(model_document(NULL, 0));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(model_document_root(NULL, 0));
    assert_errno(EINVAL);

    reset_errno();
    assert_uint_eq(0, model_document_count(NULL));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (null_node)
{
    reset_errno();
    assert_null(node_name(NULL));
    assert_errno(EINVAL);

    reset_errno();
    assert_node_size(NULL, 0);
    assert_errno(EINVAL);
}
END_TEST

START_TEST (null_document)
{
    reset_errno();
    assert_null(document_root(NULL));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (null_scalar)
{
    reset_errno();
    assert_null(scalar_value(NULL));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (null_sequence)
{
    reset_errno();
    assert_null(sequence_get(NULL, 0));
    assert_errno(EINVAL);
}
END_TEST

START_TEST (null_mapping)
{
    reset_errno();
    assert_null(mapping_get(NULL, NULL, 0));
    assert_errno(EINVAL);
    reset_errno();
    assert_mapping_has_no_key(NULL, NULL);
    assert_errno(EINVAL);
}
END_TEST

void model_setup(void)
{
    reset_errno();
    model = make_model();
    assert_not_null(model);
    assert_noerr();

    reset_errno();
    node *root = make_mapping_node();
    assert_noerr();
    assert_not_null(root);

    reset_errno();
    node *foo1 = make_scalar_node((uint8_t *)"foo1", 4, SCALAR_STRING);
    assert_noerr();
    assert_not_null(foo1);
    reset_errno();
    node *one_point_five = make_scalar_node((uint8_t *)"1.5", 4, SCALAR_REAL);
    assert_noerr();
    assert_not_null(one_point_five);
    reset_errno();
    node *one_value = make_sequence_node();
    assert_noerr();
    assert_not_null(one_value);
    reset_errno();
    sequence_add(one_value, foo1);
    assert_noerr();
    reset_errno();
    sequence_add(one_value, one_point_five);
    assert_noerr();
    reset_errno();
    mapping_put(root, (uint8_t *)"one", 3, one_value);
    assert_noerr();
    
    reset_errno();
    node *two_value = make_scalar_node((uint8_t *)"foo2", 4, SCALAR_STRING);
    assert_noerr();
    assert_not_null(two_value);
    reset_errno();
    mapping_put(root, (uint8_t *)"two", 3, two_value);
    assert_noerr();
    
    reset_errno();
    node *three_value = make_scalar_node((uint8_t *)"false", 5, SCALAR_BOOLEAN);
    assert_noerr();
    assert_not_null(three_value);
    reset_errno();
    mapping_put(root, (uint8_t *)"three", 5, three_value);
    assert_noerr();

    reset_errno();
    node *four_value = make_scalar_node((uint8_t *)"true", 4, SCALAR_BOOLEAN);
    assert_noerr();
    assert_not_null(four_value);
    reset_errno();
    mapping_put(root, (uint8_t *)"four", 4, four_value);
    assert_noerr();

    reset_errno();
    node *document = make_document_node();
    document_set_root(document, root);
    assert_noerr();
    assert_not_null(document);
    reset_errno();
    model_add(model, document);
    assert_noerr();
}

void model_teardown(void)
{
    model_free(model);
}

START_TEST (constructors)
{
    reset_errno();
    node *s = make_scalar_node(NULL, 0, SCALAR_STRING);
    assert_noerr();
    assert_not_null(s);
    node_free(s);

    reset_errno();
    s = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    assert_noerr();
    assert_not_null(s);
    
    reset_errno();
    node *d = make_document_node();
    document_set_root(d, s);
    assert_noerr();
    assert_not_null(d);
    node_free(d); // N.B. - this will also free `s'
}
END_TEST

START_TEST (document)
{
    reset_errno();
    size_t c = model_document_count(model);
    assert_noerr();
    assert_uint_eq(1, c);
    
    reset_errno();
    node *d = model_document(model, 0);
    assert_noerr();
    assert_not_null(d);
    
    reset_errno();
    node *bogus = model_document(model, 1);
    assert_errno(EINVAL);
    assert_null(bogus);
    
    reset_errno();
    node *r1 = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r1);
    assert_node_kind(r1, MAPPING);

    reset_errno();
    node *r2 = document_root(d);
    assert_noerr();
    assert_not_null(r2);
    assert_ptr_eq(r1, r2);
}
END_TEST

START_TEST (nodes)
{
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    reset_errno();
    unsigned char *n = node_name(r);
    assert_noerr();
    assert_null(n);

    reset_errno();
    size_t s = node_size(r);
    assert_noerr();
    assert_uint_eq(4, s);
}
END_TEST

START_TEST (scalar)
{
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    reset_errno();
    node *s = mapping_get(r, (uint8_t *)"two", 3ul);
    assert_noerr();
    assert_not_null(s);
    assert_node_kind(s, SCALAR);
    assert_scalar_value(s, "foo2");
}
END_TEST

START_TEST (scalar_boolean)
{
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    reset_errno();
    node *three = mapping_get(r, (uint8_t *)"three", 5ul);
    assert_noerr();
    assert_not_null(three);
    assert_node_kind(three, SCALAR);
    assert_true(scalar_boolean_is_false(three));
    assert_false(scalar_boolean_is_true(three));

    reset_errno();
    node *four = mapping_get(r, (uint8_t *)"four", 4ul);
    assert_noerr();
    assert_not_null(four);
    assert_node_kind(four, SCALAR);
    assert_true(scalar_boolean_is_true(four));
    assert_false(scalar_boolean_is_false(four));
}
END_TEST

START_TEST (sequence)
{
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    reset_errno();
    node *s = mapping_get(r, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(s);
    assert_node_kind(s, SEQUENCE);
    assert_node_size(s, 2);
    
    reset_errno();
    node *zero = sequence_get(s, 0);
    assert_noerr();
    assert_not_null(zero);
    assert_node_kind(zero, SCALAR);
    
    reset_errno();
    node *one = sequence_get(s, 1);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SCALAR);

    reset_errno();
    node *x = make_scalar_node((uint8_t *)"x", 1, SCALAR_STRING);
    assert_noerr();
    assert_not_null(x);
    reset_errno();
    node *y = make_scalar_node((uint8_t *)"y", 1, SCALAR_STRING);
    assert_noerr();
    assert_not_null(y);
    reset_errno();
    node *z = make_scalar_node((uint8_t *)"z", 1, SCALAR_STRING);
    assert_noerr();
    assert_not_null(z);
    reset_errno();
    node *xyz = make_sequence_node();
    assert_noerr();
    assert_not_null(xyz);
    assert_uint_eq(0, xyz->content.size);

    reset_errno();
    sequence_add(xyz, x);
    assert_noerr();
    reset_errno();
    sequence_add(xyz, y);
    assert_noerr();
    reset_errno();
    sequence_add(xyz, z);
    assert_noerr();
    assert_uint_eq(3, xyz->content.size);
    assert_ptr_eq(x, sequence_get(xyz, 0));
    assert_ptr_eq(y, sequence_get(xyz, 1));
    assert_ptr_eq(z, sequence_get(xyz, 2));
    node_free(xyz);
}
END_TEST

START_TEST (mapping)
{
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    assert_node_size(r, 4);

    assert_mapping_has_key(r, "two");
    assert_mapping_has_no_key(r, "bogus");

    assert_not_null(mapping_get(r, (uint8_t *)"two", 3ul));
    assert_null(mapping_get(r, (uint8_t *)"bogus", 5ul));

    reset_errno();
    node *scalar_value = mapping_get(r, (uint8_t *)"two", 3);
    assert_noerr();
    assert_not_null(scalar_value);
    assert_node_kind(scalar_value, SCALAR);
}
END_TEST

START_TEST (sequence_iteration)
{
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    
    reset_errno();
    node *s = mapping_get(r, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(s);
    assert_node_kind(s, SEQUENCE);
    assert_node_size(s, 2);
    
    size_t count = 0;
    reset_errno();
    assert_true(sequence_iterate(s, check_sequence, &count));
    assert_noerr();
    assert_uint_eq(2, count);
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
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    
    reset_errno();
    node *s = mapping_get(r, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(s);
    assert_node_kind(s, SEQUENCE);
    assert_node_size(s, 2);
    
    size_t count = 0;
    reset_errno();
    assert_false(sequence_iterate(s, fail_sequence, &count));
    assert_noerr();
    assert_uint_eq(1, count);
}
END_TEST

bool fail_sequence(node *each __attribute((unused)), void *context)
{
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
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    assert_node_size(r, 4);
    
    reset_errno();
    size_t count = 0;
    assert_true(mapping_iterate(r, check_mapping, &count));
    assert_uint_eq(4, count);
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
    reset_errno();
    node *r = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    assert_node_size(r, 4);
    
    size_t count = 0;
    reset_errno();
    assert_false(mapping_iterate(r, fail_mapping, &count));
    assert_noerr();
    assert_uint_eq(2, count);
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

    Suite *suite = suite_create("Model");
    suite_add_tcase(suite, bad_input);
    suite_add_tcase(suite, basic);
    suite_add_tcase(suite, iteration);
    
    return suite;
}
