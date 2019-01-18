#include "nodelist.h"
#include "test.h"
#include "test_document.h"
#include "test_nodelist.h"

static Nodelist *list_fixture;

static void nodelist_setup(void)
{
    list_fixture = make_nodelist();

    Scalar *foo = make_scalar_string("foo");
    assert_not_null(foo);

    Scalar *bar = make_scalar_string("bar");
    assert_not_null(bar);

    nodelist_add(list_fixture, node(foo));
    assert_nodelist_length(list_fixture, 1);

    nodelist_add(list_fixture, node(bar));
    assert_nodelist_length(list_fixture, 2);
}

static bool freedom_iterator(Node *each, void *context)
{
    dispose_node(each);
    return true;
}

static void nodelist_teardown(void)
{
    nodelist_iterate(list_fixture, freedom_iterator, NULL);
    dispose_nodelist(list_fixture);
}

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

    nodelist_add(target, node(make_scalar_integer(buffer)));
    return true;
}

static bool fail_transform(Node *each, void *context, Nodelist *target)
{
    size_t *count = (size_t *)context;
    if(0 < *count)
    {
        return false;
    }
    else
    {
        (*count)++;
        nodelist_add(target, node(make_scalar_string("munky")));
        return true;
    }
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
    assert_not_null(empty_list);

    assert_null(nodelist_get(empty_list, 0));

    dispose_nodelist(empty_list);
}
END_TEST

START_TEST (bad_add)
{
    Nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);

    fprintf(stderr, "\n!!! EXPECTED PANIC from nodelist_add should follow...\n");
    nodelist_add(empty_list, NULL);
}
END_TEST

START_TEST (bad_set)
{
    Nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);

    fprintf(stderr, "\n!!! EXPECTED PANIC from nodelist_set should follow...\n");
    Scalar *s = make_scalar_string("foo");
    nodelist_set(empty_list, node(s), 0);
}
END_TEST

START_TEST (bad_iterate)
{
    Nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);

    assert_false(nodelist_iterate(NULL, NULL, NULL));

    assert_false(nodelist_iterate(empty_list, NULL, NULL));

    dispose_nodelist(empty_list);
}
END_TEST
    
START_TEST (bad_map)
{
    Nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);

    assert_null(nodelist_map(NULL, NULL, NULL));

    assert_null(nodelist_map(empty_list, NULL, NULL));

    dispose_nodelist(empty_list);
}
END_TEST
    
START_TEST (bad_map_into)
{
    Nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);

    assert_null(nodelist_map_into(NULL, NULL, NULL, NULL));

    assert_null(nodelist_map_into(empty_list, NULL, NULL, NULL));

    assert_null(nodelist_map_into(empty_list, (nodelist_map_function)1, NULL, NULL));

    dispose_nodelist(empty_list);
}
END_TEST
    
START_TEST (ctor_dtor)
{
    Nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);

    assert_nodelist_length(empty_list, 0);
    assert_nodelist_empty(empty_list);
    
    dispose_nodelist(empty_list);
}
END_TEST

START_TEST (add)
{
    Scalar *baz = make_scalar_string("baz");
    assert_not_null(baz);

    nodelist_add(list_fixture, node(baz));
    assert_nodelist_length(list_fixture, 3);
    assert_node_equals(node(baz), nodelist_get(list_fixture, 2));
}
END_TEST

START_TEST (set)
{
    Scalar *baz = make_scalar_string("baz");
    assert_not_null(baz);

    nodelist_set(list_fixture, node(baz), 0);
    assert_nodelist_length(list_fixture, 2);
    assert_node_equals(node(baz), nodelist_get(list_fixture, 0));
}
END_TEST

START_TEST (iteration)
{
    size_t count = 0;
    assert_true(nodelist_iterate(list_fixture, check_nodelist, &count));
    assert_uint_eq(2, count);
}
END_TEST

START_TEST (fail_iteration)
{
    size_t count = 0;
    assert_false(nodelist_iterate(list_fixture, fail_nodelist, &count));
    assert_uint_eq(1, count);
}
END_TEST

START_TEST (map)
{
    size_t count = 0;

    Nodelist *result = nodelist_map(list_fixture, transform, &count);
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

    dispose_nodelist(result);
}
END_TEST

START_TEST (fail_map)
{
    size_t count = 0;
    Nodelist *result = nodelist_map(list_fixture, fail_transform, &count);
    assert_null(result);
    assert_not_null(list_fixture);
    assert_uint_eq(1, count);
}
END_TEST

Suite *nodelist_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, bad_length);
    tcase_add_test(bad_input_case, bad_get);
    tcase_add_exit_test(bad_input_case, bad_add, EXIT_FAILURE);
    tcase_add_exit_test(bad_input_case, bad_set, EXIT_FAILURE);
    tcase_add_test(bad_input_case, bad_iterate);
    tcase_add_test(bad_input_case, bad_map);
    tcase_add_test(bad_input_case, bad_map_into);
    
    TCase *basic_case = tcase_create("basic");
    tcase_add_test(basic_case, ctor_dtor);

    TCase *mutate_case = tcase_create("mutate");
    tcase_add_checked_fixture(mutate_case, nodelist_setup, nodelist_teardown);
    tcase_add_test(mutate_case, add);
    tcase_add_test(mutate_case, set);

    TCase *iterate_case = tcase_create("iterate");
    tcase_add_checked_fixture(iterate_case, nodelist_setup, nodelist_teardown);
    tcase_add_test(iterate_case, iteration);
    tcase_add_test(iterate_case, fail_iteration);
    tcase_add_test(iterate_case, map);
    tcase_add_test(iterate_case, fail_map);

    Suite *suite = suite_create("Nodelist");
    suite_add_tcase(suite, bad_input_case);
    suite_add_tcase(suite, basic_case);
    suite_add_tcase(suite, mutate_case);
    suite_add_tcase(suite, iterate_case);

    return suite;
}
