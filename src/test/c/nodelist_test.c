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

static nodelist *list;

START_TEST (bad_input)
{
    errno = 0;
    ck_assert_false(nodelist_clear(NULL));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_int_eq(0, nodelist_length(NULL));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_true(nodelist_is_empty(NULL));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_null(nodelist_get(NULL, 0));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_false(nodelist_add(NULL, NULL));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_false(nodelist_set(NULL, NULL, 0));
    ck_assert_int_eq(EINVAL, errno);

    nodelist *empty_list = make_nodelist();
    ck_assert_not_null(empty_list);

    errno = 0;
    ck_assert_null(nodelist_get(empty_list, 0));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_false(nodelist_add(empty_list, NULL));
    ck_assert_int_eq(EINVAL, errno);

    node *scalar = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);

    errno = 0;
    ck_assert_false(nodelist_set(empty_list, scalar, 0));
    ck_assert_int_eq(EINVAL, errno);

    node_free(scalar);
    nodelist_free(empty_list);
}
END_TEST

START_TEST (ctor_dtor)
{
    nodelist *empty_list = make_nodelist();
    ck_assert_not_null(empty_list);
    ck_assert_int_eq(0, nodelist_length(empty_list));
    ck_assert_true(nodelist_is_empty(empty_list));

    nodelist_free(empty_list);
    ck_assert_int_eq(0, errno);
}
END_TEST

void nodelist_setup(void)
{
    list = make_nodelist_with_capacity(2);

    errno = 0;
    node *foo = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(foo);
    errno = 0;
    node *bar = make_scalar_node((uint8_t *)"bar", 3, SCALAR_STRING);
    ck_assert_int_eq(0, errno);
    ck_assert_not_null(bar);

    nodelist_add(list, foo);
    nodelist_add(list, bar);
}

void nodelist_teardown(void)
{
    nodelist_free(list);
}

START_TEST (mutate)
{
    nodelist *mutable_list = make_nodelist();

    node *foo = make_scalar_node((uint8_t *)"foo", 3, SCALAR_STRING);
    node *bar = make_scalar_node((uint8_t *)"bar", 3, SCALAR_STRING);

    nodelist_add(mutable_list, foo);
    ck_assert_int_eq(1, nodelist_length(mutable_list));
    ck_assert_true(node_equals(foo, nodelist_get(mutable_list, 0)));

    nodelist_set(mutable_list, bar, 0);
    ck_assert_int_eq(1, nodelist_length(mutable_list));
    ck_assert_true(node_equals(bar, nodelist_get(mutable_list, 0)));

    nodelist_free(mutable_list);
    node_free(foo);
    node_free(bar);
}
END_TEST

START_TEST (iteration)
{
    size_t count = 0;
    errno = 0;
    ck_assert_true(nodelist_iterate(list, check_nodelist, &count));
    ck_assert_int_eq(0, errno);
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
    errno = 0;
    ck_assert_false(nodelist_iterate(list, fail_nodelist, &count));
    ck_assert_int_eq(0, errno);
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
    errno = 0;
    nodelist *result = nodelist_map(list, transform, &count);
    ck_assert_not_null(result);
    ck_assert_int_eq(0, errno);
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

    for(size_t i = 0; i < nodelist_length(result); i++)
    {
        node_free(nodelist_get(result, i));
    }
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
    errno = 0;
    nodelist *result = nodelist_map(list, fail_transform, &count);
    ck_assert_null(result);
    ck_assert_not_null(list);
    ck_assert_int_eq(0, errno);
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

START_TEST (map_overwrite)
{
    size_t count = 0;
    errno = 0;
    nodelist *result = nodelist_map_overwrite(list, transform, &count, list);
    ck_assert_not_null(result);
    ck_assert_int_eq(0, errno);
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
    errno = 0;
    nodelist *result = nodelist_map_overwrite(list, fail_transform, &count, list);
    ck_assert_null(result);
    ck_assert_int_eq(0, errno);
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
    tcase_add_test(basic, mutate);

    TCase *iterate = tcase_create("iterate");
    tcase_add_checked_fixture(iterate, nodelist_setup, nodelist_teardown);
    tcase_add_test(iterate, iteration);
    tcase_add_test(iterate, fail_iteration);
    tcase_add_test(iterate, map);
    tcase_add_test(iterate, fail_map);
    tcase_add_test(iterate, map_overwrite);
    tcase_add_test(iterate, fail_map_overwrite);

    Suite *nodelist = suite_create("Nodelist");
    suite_add_tcase(nodelist, basic);
    suite_add_tcase(nodelist, iterate);

    return nodelist;
}

