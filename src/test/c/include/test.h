#pragma once

#include <stdio.h>
#include <errno.h>
#include <check.h>

#include "log.h"

#if CHECK_MAJOR_VERSION == 0 && (CHECK_MINOR_VERSION < 9 || (CHECK_MINOR_VERSION == 9 && CHECK_MICRO_VERSION < 8))
#error "version of check must be 0.9.8 or later"
#endif

#if CHECK_MAJOR_VERSION == 0 && CHECK_MINOR_VERSION == 9 && CHECK_MICRO_VERSION == 8
#define CHECK_0_9_8
#endif 

#ifdef CHECK_0_9_8
#define _ck_assert_uint(X, OP, Y) do { \
  uintmax_t _ck_x = (X); \
  uintmax_t _ck_y = (Y); \
  ck_assert_msg(_ck_x OP _ck_y, "Assertion '"#X#OP#Y"' failed: "#X"==%ju, "#Y"==%ju", _ck_x, _ck_y); \
} while (0)

#define ck_assert_uint_eq(X, Y) _ck_assert_uint(X, ==, Y)
#define ck_assert_uint_ne(X, Y) _ck_assert_uint(X, !=, Y)
#define ck_assert_uint_lt(X, Y) _ck_assert_uint(X, <, Y)
#define ck_assert_uint_le(X, Y) _ck_assert_uint(X, <=, Y)
#define ck_assert_uint_gt(X, Y) _ck_assert_uint(X, >, Y)
#define ck_assert_uint_ge(X, Y) _ck_assert_uint(X, >=, Y)
#endif

#define assert_int_eq(X, Y)  ck_assert_int_eq(X, Y)
#define assert_int_ne(X, Y)  ck_assert_int_ne(X, Y)
#define assert_int_lt(X, Y)  ck_assert_int_lt(X, Y)
#define assert_int_le(X, Y)  ck_assert_int_le(X, Y)
#define assert_int_gt(X, Y)  ck_assert_int_gt(X, Y)
#define assert_int_ge(X, Y)  ck_assert_int_ge(X, Y)

#define assert_uint_eq(X, Y)  ck_assert_uint_eq(X, Y)
#define assert_uint_ne(X, Y)  ck_assert_uint_ne(X, Y)
#define assert_uint_lt(X, Y)  ck_assert_uint_lt(X, Y)
#define assert_uint_le(X, Y)  ck_assert_uint_le(X, Y)
#define assert_uint_gt(X, Y)  ck_assert_uint_gt(X, Y)
#define assert_uint_ge(X, Y)  ck_assert_uint_ge(X, Y)

#define assert_ptr_eq(X, Y)  ck_assert_msg((X) == (Y), "Assertion '" #X " == " #Y "' failed: "#X"==%p, "#Y"==%p", (X), (Y))

#define assert_null(X)              ck_assert_msg((X) == NULL, "Assertion '"#X" == NULL' failed")
#define assert_not_null(X)          ck_assert_msg((X) != NULL, "Assertion '"#X" != NULL' failed")
#define assert_buf_eq(X, N1, Y, N2) ck_assert_msg(memcmp((X), (Y), (N1) > (N2) ? (N2) : (N1)) == 0, "Assertion 'memcmp("#X", "#Y", %zu)' failed", (N1) > (N2) ? (N2) : (N1))
#define assert_true(X)              ck_assert_msg((X) == true, "Assertion '"#X" == true' failed")
#define assert_false(X)             ck_assert_msg((X) == false, "Assertion '"#X" == false' failed")

#define assert_errno(X) ck_assert_msg(errno == (X), "Assertion 'errno == "#X"' failed. errno is %d (%s)", errno, strerror(errno))
#define assert_noerr()  assert_errno(0)
#define reset_errno()   errno = 0

// test suites

Suite *master_suite(void);
Suite *loader_suite(void);
Suite *scanner_suite(void);
Suite *jsonpath_suite(void);
Suite *model_suite(void);
Suite *nodelist_suite(void);
Suite *evaluator_suite(void);

