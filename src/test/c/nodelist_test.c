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

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#ifdef __APPLE__
#define _DARWIN_SOURCE
#endif

#include <errno.h>
#include <check.h>

#include "nodelist.h"
#include "test.h"
#include "test_model.h"
#include "test_nodelist.h"

void nodelist_setup(void);
void nodelist_teardown(void);

bool  fail_nodelist(node *each, void *context);
bool  check_nodelist(node *each, void *context);
bool  transform(node *each, void *context, nodelist *target);
bool  fail_transform(node *each, void *context, nodelist *target);

#define make_scalar_string(VALUE) make_scalar_node((uint8_t *)(VALUE), strlen((VALUE)), SCALAR_STRING)
#define make_scalar_integer(VALUE) make_scalar_node((uint8_t *)(VALUE), strlen((VALUE)), SCALAR_INTEGER)

static nodelist *list;

START_TEST (bad_length)
{
    reset_errno();
    assert_nodelist_length(NULL, 0);
    assert_errno(EINVAL);
}
END_TEST

START_TEST (bad_get)
{
    reset_errno();
    assert_null(nodelist_get(NULL, 0));
    assert_errno(EINVAL);

    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_null(nodelist_get(empty_list, 0));
    assert_errno(EINVAL);

    nodelist_free(empty_list);
}
END_TEST

START_TEST (bad_add)
{
    reset_errno();
    assert_false(nodelist_add(NULL, NULL));
    assert_errno(EINVAL);

    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    assert_false(nodelist_add(empty_list, NULL));
    assert_errno(EINVAL);

    nodelist_free(empty_list);
}
END_TEST

START_TEST (bad_set)
{
    reset_errno();
    assert_false(nodelist_set(NULL, NULL, 0));
    assert_errno(EINVAL);

    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    reset_errno();
    node *scalar = make_scalar_string("foo");
    assert_not_null(scalar);
    assert_noerr();

    reset_errno();
    assert_false(nodelist_set(empty_list, scalar, 0));
    assert_errno(EINVAL);

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
    assert_errno(EINVAL);

    reset_errno();
    assert_false(nodelist_iterate(empty_list, NULL, NULL));
    assert_errno(EINVAL);

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
    assert_errno(EINVAL);

    reset_errno();
    assert_null(nodelist_map(empty_list, NULL, NULL));
    assert_errno(EINVAL);

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
    assert_errno(EINVAL);

    reset_errno();
    assert_null(nodelist_map_into(empty_list, NULL, NULL, NULL));
    assert_errno(EINVAL);

    reset_errno();
    assert_null(nodelist_map_into(empty_list, (nodelist_map_function)1, NULL, NULL));
    assert_errno(EINVAL);

    nodelist_free(empty_list);
}
END_TEST
    
START_TEST (ctor_dtor)
{
    reset_errno();
    nodelist *empty_list = make_nodelist();
    assert_not_null(empty_list);
    assert_noerr();

    assert_nodelist_length(empty_list, 0);
    assert_nodelist_empty(empty_list);
    
    reset_errno();
    nodelist_free(empty_list);
    assert_noerr();
}
END_TEST

void nodelist_setup(void)
{
    list = make_nodelist();

    reset_errno();
    node *foo = make_scalar_string("foo");
    assert_noerr();
    assert_not_null(foo);

    reset_errno();
    node *bar = make_scalar_string("bar");
    assert_noerr();
    assert_not_null(bar);

    reset_errno();
    assert_true(nodelist_add(list, foo));
    assert_noerr();
    assert_nodelist_length(list, 1);

    reset_errno();
    assert_true(nodelist_add(list, bar));
    assert_noerr();
    assert_nodelist_length(list, 2);
}

static bool freedom_iterator(node *each, void *context);

static bool freedom_iterator(node *each, void *context __attribute__((unused)))
{
    node_free(each);
    return true;
}

void nodelist_teardown(void)
{
    nodelist_iterate(list, freedom_iterator, NULL);
    nodelist_free(list);
}

START_TEST (add)
{
    reset_errno();
    node *baz = make_scalar_string("baz");
    assert_not_null(baz);
    assert_noerr();

    reset_errno();
    nodelist_add(list, baz);
    assert_noerr();
    assert_nodelist_length(list, 3);
    assert_node_equals(baz, nodelist_get(list, 2));

    // N.B. not freeing baz here as it is added to `list` and will be freed by teardown
}
END_TEST

START_TEST (set)
{
    reset_errno();
    node *baz = make_scalar_string("baz");
    assert_not_null(baz);
    assert_noerr();

    reset_errno();
    assert_true(nodelist_set(list, baz, 0));
    assert_noerr();
    assert_nodelist_length(list, 2);
    assert_node_equals(baz, nodelist_get(list, 0));

    // N.B. not freeing baz here as it is added to `list` and will be freed by teardown
}
END_TEST

START_TEST (iteration)
{
    size_t count = 0;
    reset_errno();
    assert_true(nodelist_iterate(list, check_nodelist, &count));
    assert_noerr();
    assert_uint_eq(2, count);
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
    assert_uint_eq(1, count);
}
END_TEST

bool fail_nodelist(node *each __attribute__((unused)), void *context)
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

START_TEST (map)
{
    size_t count = 0;

    reset_errno();
    nodelist *result = nodelist_map(list, transform, &count);
    assert_noerr();
    assert_not_null(result);
    assert_uint_eq(2, count);
    assert_nodelist_length(result, 2);

    node *zero = nodelist_get(result, 0);
    assert_not_null(zero);
    assert_scalar_kind(zero, SCALAR_INTEGER);
    assert_scalar_value(zero, "1");

    node *one = nodelist_get(result, 1);
    assert_not_null(one);
    assert_scalar_kind(one, SCALAR_INTEGER);
    assert_scalar_value(one, "2");

    nodelist_free(result);
}
END_TEST

bool transform(node *each, void *context, nodelist *target)
{
    assert_not_null(each);
    size_t *count = (size_t *)context;
    (*count)++;
    char *value;
    int result = asprintf(&value, "%zd", *count);
    assert_int_ne(-1, result);
    return nodelist_add(target, make_scalar_integer(value));
}

START_TEST (fail_map)
{
    size_t count = 0;
    reset_errno();
    nodelist *result = nodelist_map(list, fail_transform, &count);
    assert_null(result);
    assert_not_null(list);
    assert_noerr();
    assert_uint_eq(1, count);
}
END_TEST

bool fail_transform(node *each __attribute__((unused)), void *context, nodelist *target)
{
    size_t *count = (size_t *)context;
    if(0 < *count)
    {
        return false;
    }
    else
    {
        (*count)++;
        return nodelist_add(target, make_scalar_string("munky"));
    }
}

Suite *nodelist_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, bad_length);
    tcase_add_test(bad_input_case, bad_get);
    tcase_add_test(bad_input_case, bad_add);
    tcase_add_test(bad_input_case, bad_set);
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

