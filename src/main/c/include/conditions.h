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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

extern const void * SENTINEL;

#define VOID_RETURN 

#define ENSURE_NONNULL(ERR_RESULT, ERRNO, ...)                          \
    if(precond_is_null(__VA_ARGS__, SENTINEL))                          \
    {                                                                   \
        errno = 0 == errno ? (ERRNO) : errno;                           \
        return ERR_RESULT;                                              \
    }

#define ENSURE_THAT(ERR_RESULT, ERRNO, ...)                             \
    if(precond_is_false(__VA_ARGS__, -1))                               \
    {                                                                   \
        errno = 0 == errno ? (ERRNO) : errno;                           \
        return ERR_RESULT;                                              \
    }

// common preconditions
#define PRECOND_NONNULL_ELSE_NULL(...) ENSURE_NONNULL(NULL, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_FALSE(...) ENSURE_NONNULL(false, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_TRUE(...) ENSURE_NONNULL(true, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_VOID(...) ENSURE_NONNULL(VOID_RETURN, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_ZERO(...) ENSURE_NONNULL(0, EINVAL, __VA_ARGS__)

#define PRECOND_ELSE_NULL(...) ENSURE_THAT(NULL, EINVAL, __VA_ARGS__)
#define PRECOND_ELSE_FALSE(...) ENSURE_THAT(false, EINVAL, __VA_ARGS__)
#define PRECOND_ELSE_ZERO(...) ENSURE_THAT(0, EINVAL, __VA_ARGS__)

// common invariants
#define ENSURE_NONNULL_ELSE_NULL(ERRNO, ...) ENSURE_NONNULL(NULL, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_FALSE(ERRNO, ...) ENSURE_NONNULL(false, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_TRUE(ERRNO, ...) ENSURE_NONNULL(true, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_VOID(ERRNO, ...) ENSURE_NONNULL(VOID_RETURN, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_ZERO(ERRNO, ...) ENSURE_NONNULL(0, ERRNO, __VA_ARGS__)

#define ENSURE_ELSE_NULL(ERRNO, ...) ENSURE_THAT(NULL, ERRNO, __VA_ARGS__)
#define ENSURE_ELSE_FALSE(ERRNO, ...) ENSURE_THAT(false, ERRNO, __VA_ARGS__)


bool precond_is_null(const void *first, ...);
bool precond_is_false(int_fast8_t first, ...);
