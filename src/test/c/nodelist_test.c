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

    nodelist *list = make_nodelist();
    ck_assert_not_null(list);

    errno = 0;
    ck_assert_null(nodelist_get(list, 0));
    ck_assert_int_eq(EINVAL, errno);

    errno = 0;
    ck_assert_false(nodelist_add(list, NULL));
    ck_assert_int_eq(EINVAL, errno);

    node *scalar = make_scalar_node((uint8_t *)"foo", 3);

    errno = 0;
    ck_assert_false(nodelist_set(list, scalar, 0));
    ck_assert_int_eq(EINVAL, errno);

    node_free(scalar);
    nodelist_free(list);
}
END_TEST

START_TEST (ctor_dtor)
{
    nodelist *list = make_nodelist();
    ck_assert_not_null(list);
    ck_assert_int_eq(0, nodelist_length(list));
    ck_assert_true(nodelist_is_empty(list));

    nodelist_free(list);
    ck_assert_int_eq(0, errno);
}
END_TEST

START_TEST (mutate)
{
    nodelist *list = make_nodelist();

    node *foo = make_scalar_node((uint8_t *)"foo", 3);
    node *bar = make_scalar_node((uint8_t *)"bar", 3);

    nodelist_add(list, foo);
    ck_assert_int_eq(1, nodelist_length(list));
    ck_assert_true(node_equals(foo, nodelist_get(list, 0)));

    nodelist_set(list, bar, 0);
    ck_assert_int_eq(1, nodelist_length(list));
    ck_assert_true(node_equals(bar, nodelist_get(list, 0)));

    nodelist_free(list);
    node_free(foo);
    node_free(bar);
}
END_TEST

Suite *nodelist_suite(void)
{
    TCase *basic = tcase_create("basic");
    tcase_add_test(basic, bad_input);
    tcase_add_test(basic, ctor_dtor);
    tcase_add_test(basic, mutate);

    Suite *nodelist = suite_create("Nodelist");
    suite_add_tcase(nodelist, basic);

    return nodelist;
}

