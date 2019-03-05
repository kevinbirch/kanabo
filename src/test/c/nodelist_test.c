#include <stdio.h>

#include "nodelist.h"

#include "builders.h"
#include "test.h"
#include "test_document.h"
#include "test_nodelist.h"

static bool fail_nodelist(Node *each, void *context)
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

static bool check_nodelist(Node *each, void *context)
{
    assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;

    return true;
}

static bool transform(Node *each, void *context, Nodelist *target)
{
    assert_not_null(each);

    size_t *count = (size_t *)context;
    (*count)++;

    char buffer[32];
    int result = snprintf(buffer, 32, "%zd", *count);
    assert_int_ne(-1, result);

    nodelist_add(target, integer(buffer));

    return true;
}

static bool fail_transform(Node *each, void *context, Nodelist *target)
{
    Scalar *scalar = scalar(each);
    if(!strequ(scalar_value(scalar), "foo"))
    {
        return false;
    }
    nodelist_add(target, each);

    return true;
}

START_TEST (bad_length)
{
    assert_nodelist_length(NULL, 0);
}
END_TEST

START_TEST (bad_get)
{
    assert_null(nodelist_get(NULL, 0));

    Nodelist *empty_list = make_nodelist();
    assert_null(nodelist_get(empty_list, 0));

    dispose_nodelist(empty_list);
}
END_TEST

START_TEST (bad_iterate)
{
    Nodelist *empty_list = make_nodelist();
    assert_false(nodelist_iterate(NULL, NULL, NULL));

    assert_false(nodelist_iterate(empty_list, NULL, NULL));

    dispose_nodelist(empty_list);
}
END_TEST
    
START_TEST (bad_map)
{
    Nodelist *empty_list = make_nodelist();
    assert_null(nodelist_map(NULL, NULL, NULL));

    assert_null(nodelist_map(empty_list, NULL, NULL));

    dispose_nodelist(empty_list);
}
END_TEST

START_TEST (ctor_dtor)
{
    Nodelist *empty_list = make_nodelist();
    assert_nodelist_length(empty_list, 0);
    assert_nodelist_empty(empty_list);
    
    dispose_nodelist(empty_list);
}
END_TEST

START_TEST (add)
{
    Nodelist *fixture = make_nodelist_of(2, string("foo"), string("bar"));

    Node *baz = string("baz");
    nodelist_add(fixture, baz);
    assert_nodelist_length(fixture, 3);
    assert_node_equals(node(baz), nodelist_get(fixture, 2));

    nodelist_destroy(fixture);
}
END_TEST

START_TEST (set)
{
    Nodelist *fixture = make_nodelist_of(2, string("xxx"), string("yyy"));

    Node *baz = string("baz");
    Node *previous = nodelist_set(fixture, baz, 0);
    assert_not_null(previous);
    assert_nodelist_length(fixture, 2);
    assert_node_equals(node(baz), nodelist_get(fixture, 0));

    dispose_node(previous);
    nodelist_destroy(fixture);
}
END_TEST

START_TEST (iteration)
{
    Nodelist *fixture = make_nodelist_of(2, string("foo"), string("bar"));

    size_t count = 0;
    assert_true(nodelist_iterate(fixture, check_nodelist, &count));
    assert_uint_eq(2, count);

    nodelist_destroy(fixture);
}
END_TEST

START_TEST (fail_iteration)
{
    Nodelist *fixture = make_nodelist_of(2, string("foo"), string("bar"));

    size_t count = 0;
    assert_false(nodelist_iterate(fixture, fail_nodelist, &count));
    assert_uint_eq(1, count);

    nodelist_destroy(fixture);
}
END_TEST

START_TEST (basic_map)
{
    Nodelist *fixture = make_nodelist_of(2, string("foo"), string("bar"));
    size_t count = 0;

    Nodelist *result = nodelist_map(fixture, transform, &count);
    assert_not_null(result);
    assert_uint_eq(2, count);
    assert_nodelist_length(result, 2);

    Node *zero = nodelist_get(result, 0);
    assert_not_null(zero);
    assert_scalar_kind(zero, SCALAR_INTEGER);
    assert_scalar_value(zero, "1");

    Node *one = nodelist_get(result, 1);
    assert_not_null(one);
    assert_scalar_kind(one, SCALAR_INTEGER);
    assert_scalar_value(one, "2");

    nodelist_destroy(result);
    nodelist_destroy(fixture);
}
END_TEST

START_TEST (fail_map)
{
    Nodelist *fixture = make_nodelist_of(2, string("foo"), string("bar"));

    Nodelist *result = nodelist_map(fixture, fail_transform, NULL);
    assert_null(result);

    nodelist_destroy(fixture);
}
END_TEST

Suite *nodelist_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, bad_length);
    tcase_add_test(bad_input_case, bad_get);
    tcase_add_test(bad_input_case, bad_iterate);
    tcase_add_test(bad_input_case, bad_map);
    
    TCase *basic_case = tcase_create("basic");
    tcase_add_test(basic_case, ctor_dtor);

    TCase *mutate_case = tcase_create("mutate");
    tcase_add_test(mutate_case, add);
    tcase_add_test(mutate_case, set);

    TCase *iterate_case = tcase_create("iterate");
    tcase_add_test(iterate_case, iteration);
    tcase_add_test(iterate_case, fail_iteration);
    tcase_add_test(iterate_case, basic_map);
    tcase_add_test(iterate_case, fail_map);

    Suite *suite = suite_create("Nodelist");
    suite_add_tcase(suite, bad_input_case);
    suite_add_tcase(suite, basic_case);
    suite_add_tcase(suite, mutate_case);
    suite_add_tcase(suite, iterate_case);

    return suite;
}
