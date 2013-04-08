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

#ifndef PRECONDITIONS_H
#define PRECONDITIONS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

extern const void * SENTINEL;

#define VOID_RETURN

#define STRINGIFY(x) #x

#define ENSURE_NONNULL(ERR_RESULT, ...)                                 \
    _Pragma(STRINGIFY(GCC diagnostic push))                             \
    _Pragma(STRINGIFY(GCC diagnostic ignored "-Wincompatible-pointer-types")) \
    if(is_null(__VA_ARGS__, SENTINEL))                                  \
    {                                                                   \
        errno = EINVAL;                                                 \
        return ERR_RESULT;                                              \
    }                                                                   \
    _Pragma(STRINGIFY(GCC diagnostic pop))

#define ENSURE_THAT(ERR_RESULT, ...)                                    \
    if(is_false(__VA_ARGS__, -1))                                       \
    {                                                                   \
        errno = EINVAL;                                                 \
        return ERR_RESULT;                                              \
    }

#define ENSURE_NONNULL_ELSE_NULL(...) ENSURE_NONNULL(NULL, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_FALSE(...) ENSURE_NONNULL(false, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_TRUE(...) ENSURE_NONNULL(true, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_VOID(...) ENSURE_NONNULL(VOID_RETURN, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_ZERO(...) ENSURE_NONNULL(0, __VA_ARGS__)

#define ENSURE_COND_ELSE_NULL(...) ENSURE_THAT(NULL, __VA_ARGS__)
#define ENSURE_COND_ELSE_FALSE(...) ENSURE_THAT(false, __VA_ARGS__)


bool is_null(void * first, ...);
bool is_false(int_fast8_t first, ...);



#endif
