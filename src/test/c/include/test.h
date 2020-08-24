#pragma once

#include <stdio.h>
#include <check.h>

#undef fail

#include "log.h"
#include "maybe.h"

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
  ck_assert_msg(_ck_x OP _ck_y, "'"#X#OP#Y"' failed: "#X"==%ju, "#Y"==%ju", _ck_x, _ck_y); \
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

#define assert_ptr_eq(X, Y)  ck_assert_msg((X) == (Y), "'" #X " == " #Y "' failed: "#X"==%p, "#Y"==%p", (X), (Y))

#define assert_null(X)              ck_assert_msg((X) == NULL, "'"#X" == NULL' failed")
#define assert_not_null(X)          ck_assert_msg((X) != NULL, "'"#X" != NULL' failed")
#define assert_buf_eq(X, N1, Y, N2) ck_assert_msg(memcmp((X), (Y), (N1) > (N2) ? (N2) : (N1)) == 0, "'memcmp("#X", "#Y", %zu)' failed", (N1) > (N2) ? (N2) : (N1))
#define assert_true(X)              ck_assert_msg((X) == true, "'"#X" == true' failed")
#define assert_false(X)             ck_assert_msg((X) == false, "'"#X" == false' failed")

#define assert_nothing(X) ck_assert_msg(is_nothing((X)), "'is_nothing("#X") failed.")
#define assert_just(X) ck_assert_msg(is_just((X)), "'is_just("#X") failed.")

// test suites

Suite *master_suite(void);
Suite *loader_suite(void);
Suite *scanner_suite(void);
Suite *parser_suite(void);
Suite *model_suite(void);
Suite *nodelist_suite(void);
Suite *evaluator_suite(void);
