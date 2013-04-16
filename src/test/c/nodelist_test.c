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

#include <errno.h>
#include <check.h>

#include "nodelist.h"
#include "test.h"

void nodelist_setup(void);
void nodelist_teardown(void);

bool fail_nodelist(node *each, void *context);
bool check_nodelist(node *each, void *context);
node *transform(node *each, void *context);
node *fail_transform(node *each, void *context);
bool flat_transform(node *each, void *context, nodelist *target);
bool fail_flat_transform(node *each, void *context, nodelist *target);

static nodelist *list;

START_TEST (bad_clear)
{
    reset_errno();
    assert_false(nodelist_clear(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (bad_length)
{
    reset_errno();
    assert_int_eq(0, nodelist_length(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (bad_is_empty)
{
    reset_errno();
    assert_true(nodelist_is_empty(NULL));
    assert_int_eq(EINVAL, errno);
}
END_TEST

START_TEST (bad_get)
{
    reset_errno();
    assert_null(nodelist_get(NULL, 0));
    assert_int_eq(EINVAL, errno);

    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_null(nodelist_get(empty_list, 0));
    assert_int_eq(EINVAL, errno);

    nodelist_free(empty_list);
}
END_TEST

START_TEST (bad_add)
{
    reset_errno();
    assert_false(nodelist_add(NULL, NULL));
    assert_int_eq(EINVAL, errno);

    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_false(nodelist_add(empty_list, NULL));
    assert_int_eq(EINVAL, errno);

    nodelist_free(empty_list);
}
END_TEST

START_TEST (bad_set)
{
    reset_errno();
    assert_false(nodelist_set(NULL, NULL, 0));
    assert_int_eq(EINVAL, errno);

    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    node *scalar = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    assert_not_null(scalar);
    assert_noerr();

    reset_errno();
    assert_false(nodelist_set(empty_list, scalar, 0));
    assert_int_eq(EINVAL, errno);

    node_free(scalar);
    nodelist_free(empty_list);
}
END_TEST

START_TEST (bad_iterate)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_false(nodelist_iterate(NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_false(nodelist_iterate(empty_list, NULL, NULL));
    assert_int_eq(EINVAL, errno);

    nodelist_free(empty_list);
}
END_TEST
    
START_TEST (bad_map)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_null(nodelist_map(NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_map(empty_list, NULL, NULL));
    assert_int_eq(EINVAL, errno);

    nodelist_free(empty_list);
}
END_TEST
    
START_TEST (bad_map_into)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_null(nodelist_map_into(NULL, NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_map_into(empty_list, NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_map_into(empty_list, (nodelist_to_one_function)1, NULL, NULL));
    assert_int_eq(EINVAL, errno);

    nodelist_free(empty_list);
}
END_TEST
    
START_TEST (bad_map_overwrite)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_null(nodelist_map_overwrite(NULL, NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_map_overwrite(empty_list, NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_map_overwrite(empty_list, (nodelist_to_one_function)1, NULL, NULL));
    assert_int_eq(EINVAL, errno);

    reset_errno();
    nodelist *one_item_list = make_nodelist_with_capacity(1);
    assert_noerr();
    assert_not_null(one_item_list);

    reset_errno();
    node *scalar = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    assert_not_null(scalar);
    assert_noerr();

    reset_errno();
    nodelist_add(one_item_list, scalar);
    assert_int_eq(1, nodelist_length(one_item_list));
    assert_noerr();

    reset_errno();
    assert_null(nodelist_map_overwrite(one_item_list, (nodelist_to_one_function)1, NULL, empty_list));
    assert_int_eq(EINVAL, errno);
    
    node_free(scalar);
    nodelist_free(empty_list);
    nodelist_free(one_item_list);
}
END_TEST

START_TEST (bad_flatmap)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_null(nodelist_flatmap(NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_flatmap(empty_list, NULL, NULL));
    assert_int_eq(EINVAL, errno);

    nodelist_free(empty_list);
}
END_TEST
    
START_TEST (bad_flatmap_into)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_null(nodelist_flatmap_into(NULL, NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_flatmap_into(empty_list, NULL, NULL, NULL));
    assert_int_eq(EINVAL, errno);
    reset_errno();
    assert_null(nodelist_flatmap_into(empty_list, (nodelist_to_many_function)1, NULL, NULL));
    assert_int_eq(EINVAL, errno);

    nodelist_free(empty_list);
}
END_TEST
    
START_TEST (ctor_dtor)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();
    assert_int_eq(0, nodelist_length(empty_list));
    assert_true(nodelist_is_empty(empty_list));

    reset_errno();
    nodelist_free(empty_list);
    assert_noerr();
}
END_TEST

void nodelist_setup(void)
{
    list = make_nodelist_with_capacity(2);

    reset_errno();
    node *foo = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    assert_noerr();
    assert_not_null(foo);
    reset_errno();
    node *bar = make_scalar_node((uint8_t *)"bar", 3, SCALAR_STRING);
    assert_noerr();
    assert_not_null(bar);

    reset_errno();
    assert_true(nodelist_add(list, foo));
    assert_noerr();
    assert_int_eq(1, nodelist_length(list));
    assert_true(nodelist_add(list, bar));
    assert_noerr();
    assert_int_eq(2, nodelist_length(list));
}

void nodelist_teardown(void)
{
    nodelist_free_nodes(list);
    nodelist_free(list);
}

START_TEST (add)
{
    reset_errno();
    node *baz = make_scalar_node((uint8_t *)"baz", 3, SCALAR_STRING);
    assert_not_null(baz);
    assert_noerr();

    reset_errno();
    nodelist_add(list, baz);
    assert_noerr();
    assert_int_eq(3, nodelist_length(list));
    assert_true(node_equals(baz, nodelist_get(list, 2)));

    // N.B. not freeing baz here as it is added to `list` and will be freed by teardown
}
END_TEST

START_TEST (set)
{
    reset_errno();
    node *baz = make_scalar_node((uint8_t *)"baz", 3, SCALAR_STRING);
    assert_not_null(baz);
    assert_noerr();

    reset_errno();
    assert_true(nodelist_set(list, baz, 0));
    assert_noerr();
    assert_int_eq(2, nodelist_length(list));
    assert_true(node_equals(baz, nodelist_get(list, 0)));

    // N.B. not freeing baz here as it is added to `list` and will be freed by teardown
}
END_TEST

START_TEST (add_all)
{
    reset_errno();
    node *baz = make_scalar_node((uint8_t *)"baz", 3, SCALAR_STRING);
    assert_not_null(baz);
    assert_noerr();

    reset_errno();
    node *quaz = make_scalar_node((uint8_t *)"quaz", 4, SCALAR_STRING);
    assert_not_null(baz);
    assert_noerr();

    reset_errno();
    nodelist *other_list = make_nodelist_with_capacity(2);
    assert_noerr();
    assert_not_null(other_list);

    reset_errno();
    assert_true(nodelist_add(other_list, baz));
    assert_noerr();
    assert_int_eq(1, nodelist_length(other_list));

    reset_errno();
    assert_true(nodelist_add(other_list, quaz));
    assert_noerr();
    assert_int_eq(2, nodelist_length(other_list));

    reset_errno();
    assert_true(nodelist_add_all(list, other_list));
    assert_noerr();
    assert_int_eq(4, nodelist_length(list));
    assert_int_eq(2, nodelist_length(other_list));
    
    // N.B. not freeing baz and quaz here as they are added to `list` and will be freed by teardown
    nodelist_free(other_list);
}
END_TEST

START_TEST (iteration)
{
    size_t count = 0;
    reset_errno();
    assert_true(nodelist_iterate(list, check_nodelist, &count));
    assert_noerr();
    assert_int_eq(2, count);
}
END_TEST

bool check_nodelist(node *each, void *context)
{
    assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    return true;
}

START_TEST (fail_iteration)
{
    size_t count = 0;
    reset_errno();
    assert_false(nodelist_iterate(list, fail_nodelist, &count));
    assert_noerr();
    assert_int_eq(1, count);
}
END_TEST

bool fail_nodelist(node *each, void *context)
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

START_TEST (map)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_map(list, transform, &count);
    assert_not_null(result);
    assert_noerr();
    assert_int_eq(2, count);
    assert_int_eq(2, nodelist_length(result));
    node *zero = nodelist_get(result, 0);
    assert_not_null(zero);
    assert_int_eq(SCALAR_NUMBER, scalar_get_kind(zero));
    assert_buf_eq("1", 1, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(result, 1);
    assert_not_null(one);
    assert_int_eq(SCALAR_NUMBER, scalar_get_kind(one));
    assert_buf_eq("2", 1, scalar_get_value(one), node_get_size(one));

    nodelist_free_nodes(result);
    nodelist_free(result);
}
END_TEST

node *transform(node *each, void *context)
{
    assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    char *value;
    asprintf(&value, "%zd", *count);
    return make_scalar_node((uint8_t *)value, strlen(value), SCALAR_NUMBER);
}

START_TEST (fail_map)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_map(list, fail_transform, &count);
    assert_null(result);
    assert_not_null(list);
    assert_noerr();
    assert_int_eq(1, count);
}
END_TEST

node *fail_transform(node *each, void *context)
{
#pragma unused(each)

    size_t *count = (size_t *)context;
    if(0 < *count)
    {
        return NULL;
    }
    else
    {
        (*count)++;
        return make_scalar_node((uint8_t *)"munky", 5, SCALAR_STRING);
    }
}

START_TEST (flatmap)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_flatmap(list, flat_transform, &count);
    assert_not_null(result);
    assert_noerr();
    assert_int_eq(2, count);
    assert_int_eq(2, nodelist_length(result));
    node *zero = nodelist_get(result, 0);
    assert_not_null(zero);
    assert_int_eq(SCALAR_NUMBER, scalar_get_kind(zero));
    assert_buf_eq("1", 1, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(result, 1);
    assert_not_null(one);
    assert_int_eq(SCALAR_NUMBER, scalar_get_kind(one));
    assert_buf_eq("2", 1, scalar_get_value(one), node_get_size(one));

    nodelist_free_nodes(result);
    nodelist_free(result);
}
END_TEST

bool flat_transform(node *each, void *context, nodelist *target)
{
    assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    char *value;
    asprintf(&value, "%zd", *count);
    node *scalar = make_scalar_node((uint8_t *)value, strlen(value), SCALAR_NUMBER);
    nodelist_add(target, scalar);
    
    return true;
}

START_TEST (fail_flatmap)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_flatmap(list, fail_flat_transform, &count);
    assert_null(result);
    assert_not_null(list);
    assert_noerr();
    assert_int_eq(1, count);
}
END_TEST

bool fail_flat_transform(node *each, void *context, nodelist *target)
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
        node *scalar = make_scalar_node((uint8_t *)"munky", 5, SCALAR_STRING);
        nodelist_add(target, scalar);
        return true;
    }
}

START_TEST (map_overwrite)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_map_overwrite(list, transform, &count, list);
    assert_not_null(result);
    assert_noerr();
    assert_int_eq(2, count);
    assert_int_eq(2, nodelist_length(result));
    node *zero = nodelist_get(list, 0);
    assert_not_null(zero);
    assert_int_eq(SCALAR_NUMBER, scalar_get_kind(zero));
    assert_buf_eq("1", 1, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(list, 1);
    assert_not_null(one);
    assert_int_eq(SCALAR_NUMBER, scalar_get_kind(one));
    assert_buf_eq("2", 1, scalar_get_value(one), node_get_size(one));
}
END_TEST

START_TEST (fail_map_overwrite)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_map_overwrite(list, fail_transform, &count, list);
    assert_null(result);
    assert_noerr();
    assert_int_eq(1, count);
    assert_int_eq(2, nodelist_length(list));
    node *zero = nodelist_get(list, 0);
    assert_not_null(zero);
    assert_int_eq(SCALAR_STRING, scalar_get_kind(zero));
    assert_buf_eq("munky", 5, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(list, 1);
    assert_not_null(one);
    assert_int_eq(SCALAR_STRING, scalar_get_kind(one));
    assert_buf_eq("bar", 3, scalar_get_value(one), node_get_size(one));
}
END_TEST

Suite *nodelist_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, bad_clear);
    tcase_add_test(bad_input_case, bad_length);
    tcase_add_test(bad_input_case, bad_is_empty);
    tcase_add_test(bad_input_case, bad_get);
    tcase_add_test(bad_input_case, bad_add);
    tcase_add_test(bad_input_case, bad_set);
    tcase_add_test(bad_input_case, bad_iterate);
    tcase_add_test(bad_input_case, bad_map);
    tcase_add_test(bad_input_case, bad_map_into);
    tcase_add_test(bad_input_case, bad_map_overwrite);
    tcase_add_test(bad_input_case, bad_flatmap);
    tcase_add_test(bad_input_case, bad_flatmap_into);
    
    TCase *basic_case = tcase_create("basic");
    tcase_add_test(basic_case, ctor_dtor);

    TCase *mutate_case = tcase_create("mutate");
    tcase_add_checked_fixture(mutate_case, nodelist_setup, nodelist_teardown);
    tcase_add_test(mutate_case, add);
    tcase_add_test(mutate_case, set);
    tcase_add_test(mutate_case, add_all);

    TCase *iterate_case = tcase_create("iterate");
    tcase_add_checked_fixture(iterate_case, nodelist_setup, nodelist_teardown);
    tcase_add_test(iterate_case, iteration);
    tcase_add_test(iterate_case, fail_iteration);
    tcase_add_test(iterate_case, map);
    tcase_add_test(iterate_case, fail_map);
    tcase_add_test(iterate_case, map_overwrite);
    tcase_add_test(iterate_case, fail_map_overwrite);
    tcase_add_test(iterate_case, flatmap);
    tcase_add_test(iterate_case, fail_flatmap);

    Suite *nodelist_suite = suite_create("Nodelist");
    suite_add_tcase(nodelist_suite, basic_case);
    suite_add_tcase(nodelist_suite, mutate_case);
    suite_add_tcase(nodelist_suite, iterate_case);

    return nodelist_suite;
}

