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
nodelist *flat_transform(node *each, void *context);
nodelist *fail_flat_transform(node *each, void *context);

static nodelist *list;

START_TEST (bad_input)
{
    reset_errno();
    ck_assert_false(nodelist_clear(NULL));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    ck_assert_int_eq(0, nodelist_length(NULL));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    ck_assert_true(nodelist_is_empty(NULL));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    ck_assert_null(nodelist_get(NULL, 0));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    ck_assert_false(nodelist_add(NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    ck_assert_false(nodelist_set(NULL, NULL, 0));
    ck_assert_int_eq(EINVAL, errno);

    nodelist *empty_list = make_nodelist();
    ck_assert_not_null(empty_list);

    reset_errno();
    ck_assert_null(nodelist_get(empty_list, 0));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    ck_assert_false(nodelist_add(empty_list, NULL));
    ck_assert_int_eq(EINVAL, errno);

    node *scalar = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);

    reset_errno();
    ck_assert_false(nodelist_set(empty_list, scalar, 0));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    ck_assert_false(nodelist_iterate(NULL, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    reset_errno();
    ck_assert_false(nodelist_iterate(empty_list, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    
    reset_errno();
    ck_assert_null(nodelist_map(NULL, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    reset_errno();
    ck_assert_null(nodelist_map(empty_list, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    
    reset_errno();
    ck_assert_null(nodelist_map_into(NULL, NULL, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    reset_errno();
    ck_assert_null(nodelist_map_into(empty_list, NULL, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    reset_errno();
    ck_assert_null(nodelist_map_into(empty_list, (nodelist_to_one_function)1, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    
    reset_errno();
    ck_assert_null(nodelist_map_overwrite(NULL, NULL, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    reset_errno();
    ck_assert_null(nodelist_map_overwrite(empty_list, NULL, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);
    reset_errno();
    ck_assert_null(nodelist_map_overwrite(empty_list, (nodelist_to_one_function)1, NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);

    reset_errno();
    nodelist *one_item_list = make_nodelist_with_capacity(1);
    ck_assert_noerr();
    ck_assert_not_null(empty_list);

    nodelist_add(one_item_list, scalar);

    reset_errno();
    ck_assert_null(nodelist_map_overwrite(one_item_list, (nodelist_to_one_function)1, NULL, empty_list));
    ck_assert_int_eq(EINVAL, errno);
    
    node_free(scalar);
    nodelist_free(empty_list);
    nodelist_free(one_item_list);
}
END_TEST

START_TEST (ctor_dtor)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    ck_assert_not_null(empty_list);
    ck_assert_noerr();
    ck_assert_int_eq(0, nodelist_length(empty_list));
    ck_assert_true(nodelist_is_empty(empty_list));

    reset_errno();
    nodelist_free(empty_list);
    ck_assert_noerr();
}
END_TEST

void nodelist_setup(void)
{
    list = make_nodelist_with_capacity(2);

    reset_errno();
    node *foo = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    ck_assert_noerr();
    ck_assert_not_null(foo);
    reset_errno();
    node *bar = make_scalar_node((uint8_t *)"bar", 3, SCALAR_STRING);
    ck_assert_noerr();
    ck_assert_not_null(bar);

    reset_errno();
    ck_assert_true(nodelist_add(list, foo));
    ck_assert_noerr();
    ck_assert_int_eq(1, nodelist_length(list));
    ck_assert_true(nodelist_add(list, bar));
    ck_assert_noerr();
    ck_assert_int_eq(2, nodelist_length(list));
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
    ck_assert_not_null(baz);
    ck_assert_noerr();

    reset_errno();
    nodelist_add(list, baz);
    ck_assert_noerr();
    ck_assert_int_eq(3, nodelist_length(list));
    ck_assert_true(node_equals(baz, nodelist_get(list, 2)));

    // N.B. not freeing baz here as it is added to `list` and will be freed by teardown
}
END_TEST

START_TEST (set)
{
    reset_errno();
    node *baz = make_scalar_node((uint8_t *)"baz", 3, SCALAR_STRING);
    ck_assert_not_null(baz);
    ck_assert_noerr();

    reset_errno();
    ck_assert_true(nodelist_set(list, baz, 0));
    ck_assert_noerr();
    ck_assert_int_eq(2, nodelist_length(list));
    ck_assert_true(node_equals(baz, nodelist_get(list, 0)));

    // N.B. not freeing baz here as it is added to `list` and will be freed by teardown
}
END_TEST

START_TEST (add_all)
{
    reset_errno();
    node *baz = make_scalar_node((uint8_t *)"baz", 3, SCALAR_STRING);
    ck_assert_not_null(baz);
    ck_assert_noerr();

    reset_errno();
    node *quaz = make_scalar_node((uint8_t *)"quaz", 4, SCALAR_STRING);
    ck_assert_not_null(baz);
    ck_assert_noerr();

    reset_errno();
    nodelist *other_list = make_nodelist_with_capacity(2);
    ck_assert_noerr();
    ck_assert_not_null(other_list);

    reset_errno();
    ck_assert_true(nodelist_add(other_list, baz));
    ck_assert_noerr();
    ck_assert_int_eq(1, nodelist_length(other_list));

    reset_errno();
    ck_assert_true(nodelist_add(other_list, quaz));
    ck_assert_noerr();
    ck_assert_int_eq(2, nodelist_length(other_list));

    reset_errno();
    ck_assert_not_null(list);
    ck_assert_not_null(other_list);
    
    ck_assert_true(nodelist_add_all(list, other_list));
    ck_assert_noerr();
    ck_assert_int_eq(4, nodelist_length(list));
    ck_assert_int_eq(2, nodelist_length(other_list));
    
    // N.B. not freeing baz and quaz here as they are added to `list` and will be freed by teardown
    nodelist_free(other_list);
}
END_TEST

START_TEST (iteration)
{
    size_t count = 0;
    reset_errno();
    ck_assert_true(nodelist_iterate(list, check_nodelist, &count));
    ck_assert_noerr();
    ck_assert_int_eq(2, count);
}
END_TEST

bool check_nodelist(node *each, void *context)
{
    ck_assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    return true;
}

START_TEST (fail_iteration)
{
    size_t count = 0;
    reset_errno();
    ck_assert_false(nodelist_iterate(list, fail_nodelist, &count));
    ck_assert_noerr();
    ck_assert_int_eq(1, count);
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
    ck_assert_not_null(result);
    ck_assert_noerr();
    ck_assert_int_eq(2, count);
    ck_assert_int_eq(2, nodelist_length(result));
    node *zero = nodelist_get(result, 0);
    ck_assert_not_null(zero);
    ck_assert_int_eq(SCALAR_NUMBER, scalar_get_kind(zero));
    ck_assert_buf_eq("1", 1, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(result, 1);
    ck_assert_not_null(one);
    ck_assert_int_eq(SCALAR_NUMBER, scalar_get_kind(one));
    ck_assert_buf_eq("2", 1, scalar_get_value(one), node_get_size(one));

    nodelist_free_nodes(result);
    nodelist_free(result);
}
END_TEST

node *transform(node *each, void *context)
{
    ck_assert_not_null(each);
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
    ck_assert_null(result);
    ck_assert_not_null(list);
    ck_assert_noerr();
    ck_assert_int_eq(1, count);
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
    ck_assert_not_null(result);
    ck_assert_noerr();
    ck_assert_int_eq(2, count);
    ck_assert_int_eq(2, nodelist_length(result));
    node *zero = nodelist_get(result, 0);
    ck_assert_not_null(zero);
    ck_assert_int_eq(SCALAR_NUMBER, scalar_get_kind(zero));
    ck_assert_buf_eq("1", 1, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(result, 1);
    ck_assert_not_null(one);
    ck_assert_int_eq(SCALAR_NUMBER, scalar_get_kind(one));
    ck_assert_buf_eq("2", 1, scalar_get_value(one), node_get_size(one));

    nodelist_free_nodes(result);
    nodelist_free(result);
}
END_TEST

nodelist *flat_transform(node *each, void *context)
{
    ck_assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    char *value;
    asprintf(&value, "%zd", *count);
    node *scalar = make_scalar_node((uint8_t *)value, strlen(value), SCALAR_NUMBER);
    nodelist *result = make_nodelist_with_capacity(1);
    nodelist_add(result, scalar);
    
    return result;
}

START_TEST (fail_flatmap)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_flatmap(list, fail_flat_transform, &count);
    ck_assert_null(result);
    ck_assert_not_null(list);
    ck_assert_noerr();
    ck_assert_int_eq(1, count);
}
END_TEST

nodelist *fail_flat_transform(node *each, void *context)
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
        node *scalar = make_scalar_node((uint8_t *)"munky", 5, SCALAR_STRING);
        nodelist *result = make_nodelist_with_capacity(1);
        nodelist_add(result, scalar);
        return result;
    }
}

START_TEST (map_overwrite)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_map_overwrite(list, transform, &count, list);
    ck_assert_not_null(result);
    ck_assert_noerr();
    ck_assert_int_eq(2, count);
    ck_assert_int_eq(2, nodelist_length(result));
    node *zero = nodelist_get(list, 0);
    ck_assert_not_null(zero);
    ck_assert_int_eq(SCALAR_NUMBER, scalar_get_kind(zero));
    ck_assert_buf_eq("1", 1, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(list, 1);
    ck_assert_not_null(one);
    ck_assert_int_eq(SCALAR_NUMBER, scalar_get_kind(one));
    ck_assert_buf_eq("2", 1, scalar_get_value(one), node_get_size(one));
}
END_TEST

START_TEST (fail_map_overwrite)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_map_overwrite(list, fail_transform, &count, list);
    ck_assert_null(result);
    ck_assert_noerr();
    ck_assert_int_eq(1, count);
    ck_assert_int_eq(2, nodelist_length(list));
    node *zero = nodelist_get(list, 0);
    ck_assert_not_null(zero);
    ck_assert_int_eq(SCALAR_STRING, scalar_get_kind(zero));
    ck_assert_buf_eq("munky", 5, scalar_get_value(zero), node_get_size(zero));
    node *one = nodelist_get(list, 1);
    ck_assert_not_null(one);
    ck_assert_int_eq(SCALAR_STRING, scalar_get_kind(one));
    ck_assert_buf_eq("bar", 3, scalar_get_value(one), node_get_size(one));
}
END_TEST

Suite *nodelist_suite(void)
{
    TCase *basic = tcase_create("basic");
    tcase_add_test(basic, bad_input);
    tcase_add_test(basic, ctor_dtor);

    TCase *mutate = tcase_create("mutate");
    tcase_add_checked_fixture(mutate, nodelist_setup, nodelist_teardown);
    tcase_add_test(mutate, add);
    tcase_add_test(mutate, set);
    tcase_add_test(mutate, add_all);

    TCase *iterate = tcase_create("iterate");
    tcase_add_checked_fixture(iterate, nodelist_setup, nodelist_teardown);
    tcase_add_test(iterate, iteration);
    tcase_add_test(iterate, fail_iteration);
    tcase_add_test(iterate, map);
    tcase_add_test(iterate, fail_map);
    tcase_add_test(iterate, map_overwrite);
    tcase_add_test(iterate, fail_map_overwrite);
    tcase_add_test(iterate, flatmap);
    tcase_add_test(iterate, fail_flatmap);

    Suite *nodelist = suite_create("Nodelist");
    suite_add_tcase(nodelist, basic);
    suite_add_tcase(nodelist, mutate);
    suite_add_tcase(nodelist, iterate);

    return nodelist;
}

