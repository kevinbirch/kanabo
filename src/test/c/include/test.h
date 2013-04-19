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

#pragma once

#include <stdio.h>
#include <errno.h>
#include <check.h>

#include "log.h"

#define assert_int_eq(X, Y)  ck_assert_int_eq(X, Y)
#define assert_int_ne(X, Y)  ck_assert_int_ne(X, Y)
#define assert_int_lt(X, Y)  ck_assert_int_lt(X, Y)
#define assert_int_le(X, Y)  ck_assert_int_le(X, Y)
#define assert_int_gt(X, Y)  ck_assert_int_gt(X, Y)
#define assert_int_ge(X, Y)  ck_assert_int_ge(X, Y)

#define assert_ptr_eq(X, Y)  ck_assert_msg((X) == (Y), "Assertion '" #X " == " #Y "' failed: "#X"==%p, "#Y"==%p", (X), (Y))

#define assert_null(X)              ck_assert_msg((X) == NULL, "Assertion '"#X" == NULL' failed")
#define assert_not_null(X)          ck_assert_msg((X) != NULL, "Assertion '"#X" != NULL' failed")
#define assert_buf_eq(X, N1, Y, N2) ck_assert_msg(memcmp((X), (Y), (N1) > (N2) ? (N2) : (N1)) == 0, "Assertion 'memcmp("#X", "#Y", %zd)' failed", (N1) > (N2) ? (N2) : (N1))
#define assert_true(X)              ck_assert_msg((X) == true, "Assertion '"#X" == true' failed")
#define assert_false(X)             ck_assert_msg((X) == false, "Assertion '"#X" == false' failed")

#define assert_errno(X) ck_assert_msg(errno == (X), "Assertion 'errno == "#X"' failed. errno is %d (%s)", errno, strerror(errno))
#define assert_noerr()  assert_errno(0)
#define reset_errno()   errno = 0

// test suites

Suite *master_suite(void);
Suite *loader_suite(void);
Suite *jsonpath_suite(void);
Suite *model_suite(void);
Suite *nodelist_suite(void);
Suite *evaluator_suite(void);

