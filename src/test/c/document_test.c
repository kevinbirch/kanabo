#include <stdio.h>

#include <check.h>

#include "builders.h"
#include "test.h"
#include "test_document.h"

static DocumentSet *model_fixture;

START_TEST (null_model)
{
    assert_null(document_set_get_root(NULL, 0));
}
END_TEST

START_TEST (null_mapping)
{
    assert_null(mapping_lookup(NULL, NULL));
}
END_TEST

static void model_setup(void)
{
    /**
      The document that will be used for testing this module is as follows:
    
      one:
        - "foo1"
        - 1.5
      two: "foo2"
      three: false
      four: true
     */
    Node *root = map(
        "one", seq(
            string("foo1"),
            real("1.5")),
        "two", string("foo2"),
        "three", string("false"),
        "four", string("true"));

    Document *doc = make_document_node();
    document_set_root(doc, root);

    model_fixture = make_document_set();
    document_set_add(model_fixture, doc);
}

static void model_teardown(void)
{
    dispose_document_set(model_fixture);
}

static bool check_sequence(Node *each, void *context)
{
    assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    return true;
}

static bool fail_sequence(Node *each, void *context)
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

static bool check_mapping(Scalar *key, Node *value, void *context)
{
    assert_not_null(key);
    assert_not_null(value);
    size_t *count = (size_t *)context;
    (*count)++;
    return true;
}

static bool fail_mapping(Scalar *key, Node *value, void *context)
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

START_TEST (constructors)
{
    Scalar *s = make_scalar_node(make_string("foo"), SCALAR_STRING);
    assert_not_null(s);
    
    Document *d = make_document_node();
    document_set_root(d, node(s));
    assert_not_null(d);
    dispose_node(node(d)); // N.B. - this will also free `s'
}
END_TEST

START_TEST (document_type)
{
    size_t c = document_set_size(model_fixture);
    assert_uint_eq(1, c);
    
    Document *d = document_set_get(model_fixture, 0);
    assert_not_null(d);
    
    Node *bogus = document_set_get(model_fixture, 1);
    assert_null(bogus);
    
    Node *r1 = document_set_get_root(model_fixture, 0);
    assert_not_null(r1);
    assert_node_kind(r1, MAPPING);

    Node *r2 = document_root(d);
    assert_not_null(r2);
    assert_ptr_eq(r1, r2);
}
END_TEST

START_TEST (nodes)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    String *n = node_name(r);
    assert_null(n);

    size_t s = node_size(r);
    assert_uint_eq(4, s);
}
END_TEST

START_TEST (scalar_type)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    String *key = make_string("two");
    Node *s = mapping_lookup(mapping(r), key);
    dispose_string(key);

    assert_not_null(s);
    assert_node_kind(s, SCALAR);
    assert_scalar_value((s), "foo2");
}
END_TEST

START_TEST (scalar_boolean)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    String *key1 = make_string("three");
    Node *three = mapping_lookup(mapping(r), key1);
    dispose_string(key1);
    assert_not_null(three);
    assert_node_kind(three, SCALAR);
    assert_true(scalar_boolean_is_false(scalar(three)));
    assert_false(scalar_boolean_is_true(scalar(three)));

    String *key2 = make_string("four");
    Node *four = mapping_lookup(mapping(r), key2);
    dispose_string(key2);
    assert_not_null(four);
    assert_node_kind(four, SCALAR);
    assert_true(scalar_boolean_is_true(scalar(four)));
    assert_false(scalar_boolean_is_false(scalar(four)));
}
END_TEST

START_TEST (sequence_type)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);

    String *key = make_string("one");
    Node *s = mapping_lookup(mapping(r), key);
    dispose_string(key);
    assert_not_null(s);
    assert_node_kind(s, SEQUENCE);
    assert_node_size(s, 2);
    
    Node *zero = sequence_get(sequence(s), 0);
    assert_not_null(zero);
    assert_node_kind(zero, SCALAR);
    
    Node *one = sequence_get(sequence(s), 1);
    assert_not_null(one);
    assert_node_kind(one, SCALAR);

    Scalar *x = make_scalar_node(make_string("x"), SCALAR_STRING);
    assert_not_null(x);
    Scalar *y = make_scalar_node(make_string("y"), SCALAR_STRING);
    assert_not_null(y);
    Scalar *z = make_scalar_node(make_string("z"), SCALAR_STRING);
    assert_not_null(z);
    Sequence *xyz = make_sequence_node();
    assert_not_null(xyz);

    sequence_add(xyz, node(x));
    sequence_add(xyz, node(y));
    sequence_add(xyz, node(z));
    assert_uint_eq(3, node_size(node(xyz)));
    assert_ptr_eq(node(x), sequence_get(xyz, 0));
    assert_ptr_eq(node(y), sequence_get(xyz, 1));
    assert_ptr_eq(node(z), sequence_get(xyz, 2));
    dispose_node(node(xyz));
}
END_TEST

START_TEST (mapping_type)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    assert_node_size(r, 4);

    String *bogus = make_string("bogus");
    assert_null(mapping_lookup(mapping(r), bogus));
    dispose_string(bogus);

    String *key = make_string("two");
    Node *scalar_value = mapping_lookup(mapping(r), key);
    dispose_string(key);
    assert_not_null(scalar_value);
    assert_node_kind(scalar_value, SCALAR);
}
END_TEST

START_TEST (sequence_iteration)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    
    String *key = make_string("one");
    Node *s = mapping_lookup(mapping(r), key);
    dispose_string(key);
    assert_not_null(s);
    assert_node_kind(s, SEQUENCE);
    assert_node_size(s, 2);
    
    size_t count = 0;
    assert_true(sequence_iterate(sequence(s), check_sequence, &count));
    assert_uint_eq(2, count);
}
END_TEST

START_TEST (fail_sequence_iteration)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    
    String *key = make_string("one");
    Node *s = mapping_lookup(mapping(r), key);
    dispose_string(key);
    assert_not_null(s);
    assert_node_kind(s, SEQUENCE);
    assert_node_size(s, 2);
    
    size_t count = 0;
    assert_false(sequence_iterate(sequence(s), fail_sequence, &count));
    assert_uint_eq(1, count);
}
END_TEST

START_TEST (mapping_iteration)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    assert_node_size(r, 4);
    
    size_t count = 0;
    assert_true(mapping_iterate(mapping(r), check_mapping, &count));
    assert_uint_eq(4, count);
}
END_TEST

START_TEST (fail_mapping_iteration)
{
    Node *r = document_set_get_root(model_fixture, 0);
    assert_not_null(r);
    assert_node_kind(r, MAPPING);
    assert_node_size(r, 4);
    
    size_t count = 0;
    assert_false(mapping_iterate(mapping(r), fail_mapping, &count));
    assert_uint_eq(2, count);
}
END_TEST

Suite *model_suite(void)
{
    TCase *bad_input = tcase_create("bad input");
    tcase_add_test(bad_input, null_model);
    tcase_add_test(bad_input, null_mapping);

    TCase *basic = tcase_create("basic");
    tcase_add_checked_fixture(basic, model_setup, model_teardown);
    tcase_add_test(basic, constructors);
    tcase_add_test(basic, document_type);
    tcase_add_test(basic, nodes);
    tcase_add_test(basic, scalar_type);
    tcase_add_test(basic, scalar_boolean);
    tcase_add_test(basic, sequence_type);
    tcase_add_test(basic, mapping_type);

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
