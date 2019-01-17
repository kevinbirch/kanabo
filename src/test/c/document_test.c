#include <stdio.h>

#include <check.h>

#include "document.h"
#include "test.h"
#include "test_document.h"

static DocumentSet *model_fixture;

START_TEST (null_model)
{
    assert_null(document_set_get_root(NULL, 0));
    assert_null(document_set_get_root(NULL, 0));
    assert_uint_eq(0, document_set_size(NULL));
}
END_TEST

START_TEST (null_node)
{
    assert_null(node_name(NULL));
    assert_node_size(NULL, 0);
}
END_TEST

START_TEST (null_mapping)
{
    assert_null(mapping_get(NULL, NULL));
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
    model_fixture = make_document_set();

    Mapping *root = make_mapping_node();

    Scalar *foo1 = make_scalar_string("foo1");
    Scalar *one_point_five = make_scalar_real("1.5");
    Sequence *one_value = make_sequence_node();
    sequence_add(one_value, node(foo1));
    sequence_add(one_value, node(one_point_five));

    String *one = make_string("one");
    mapping_put(root, one, node(one_value));

    String *two = make_string("two");
    Scalar *two_value = make_scalar_string("foo2");
    mapping_put(root, two, node(two_value));

    String *three = make_string("three");
    Scalar *three_value = make_scalar_string("false");
    mapping_put(root, three, node(three_value));

    String *four = make_string("four");
    Scalar *four_value = make_scalar_string("true");
    mapping_put(root, four, node(four_value));

    Document *doc = make_document_node();
    document_set_root(doc, node(root));
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

static bool check_mapping(String *key, Node *value, void *context)
{
    assert_not_null(key);
    assert_not_null(value);
    size_t *count = (size_t *)context;
    (*count)++;
    return true;
}

static bool fail_mapping(String *key, Node *value, void *context)
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
    Scalar *s = make_scalar_string("foo");
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

    unsigned char *n = node_name(r);
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
    Node *s = mapping_get(mapping(r), key);
    string_free(key);
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
    Node *three = mapping_get(mapping(r), key1);
    string_free(key1);
    assert_not_null(three);
    assert_node_kind(three, SCALAR);
    assert_true(scalar_boolean_is_false(scalar(three)));
    assert_false(scalar_boolean_is_true(scalar(three)));

    String *key2 = make_string("four");
    Node *four = mapping_get(mapping(r), key2);
    string_free(key2);
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
    Node *s = mapping_get(mapping(r), key);
    string_free(key);
    assert_not_null(s);
    assert_node_kind(s, SEQUENCE);
    assert_node_size(s, 2);
    
    Node *zero = sequence_get(sequence(s), 0);
    assert_not_null(zero);
    assert_node_kind(zero, SCALAR);
    
    Node *one = sequence_get(sequence(s), 1);
    assert_not_null(one);
    assert_node_kind(one, SCALAR);

    Scalar *x = make_scalar_string("x");
    assert_not_null(x);
    Scalar *y = make_scalar_string("y");
    assert_not_null(y);
    Scalar *z = make_scalar_string("z");
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
    assert_null(mapping_get(mapping(r), bogus));
    string_free(bogus);

    String *key = make_string("two");
    Node *scalar_value = mapping_get(mapping(r), key);
    string_free(key);
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
    Node *s = mapping_get(mapping(r), key);
    string_free(key);
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
    Node *s = mapping_get(mapping(r), key);
    string_free(key);
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
    tcase_add_test(bad_input, null_node);
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
