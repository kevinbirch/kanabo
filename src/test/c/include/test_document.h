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

#include "document.h"

// model assertions
#define assert_node_kind(NODE, EXPECTED)      assert_int_eq((EXPECTED), node_kind((NODE)))
#define assert_node_size(NODE, EXPECTED)      assert_uint_eq((EXPECTED), node_size((NODE)))

#define assert_node_tag(NODE, TAG) do {                                 \
        assert_not_null(NODE);                                          \
        char    *_expected_tag = (TAG);                                 \
        size_t   _expected_len = strlen(_expected_tag);                 \
        node    *_assert_node = (NODE);                                 \
        uint8_t *_assert_name = node_name(_assert_node);                \
        size_t   _assert_name_len = strlen((char *)_assert_name);       \
        assert_int_eq(_expected_len, _assert_name_len);                 \
        char     _actual_tag[_assert_name_len + 1];                     \
        memcpy(&_actual_tag, _assert_name, _assert_name_len);           \
        _actual_tag[_assert_name_len] = '\0';                           \
        bool _assert_result = memcmp(_expected_tag, _actual_tag, _expected_len) == 0; \
        ck_assert_msg(_assert_result, "Assertion 'memcmp("#TAG", \"%s\", %zu)' failed", _actual_tag, _expected_len); \
    } while(0)

#define assert_node_equals(X, Y)              assert_true(node_equals((X), (Y)))

#define assert_mapping_has_key(NODE, KEY)     assert_not_null(mapping_get((NODE), (uint8_t *)(KEY), NULL == (KEY) ? 0 : strlen(KEY)))
#define assert_mapping_has_no_key(NODE, KEY)  assert_null(mapping_get((NODE), (uint8_t *)(KEY), NULL == (KEY) ? 0 : strlen(KEY)))

#define assert_scalar_kind(NODE, EXPECTED)    assert_int_eq(EXPECTED, scalar_kind((NODE)))

#define assert_scalar_value(NODE, VALUE) do {                           \
        assert_not_null(NODE);                                          \
        assert_node_kind(NODE, SCALAR);                                 \
        char    *_expected_value = (VALUE);                             \
        size_t   _expected_len = strlen(_expected_value);               \
        node    *_assert_node = (NODE);                                 \
        uint8_t *_assert_value = scalar_value(_assert_node);            \
        size_t   _actual_len = node_size(_assert_node);                 \
        char     _actual_value[_actual_len + 1];                        \
        memcpy(&_actual_value, _assert_value, _actual_len);             \
        _actual_value[_actual_len] = '\0';                              \
        bool _assert_result = memcmp(_expected_value, _actual_value, _expected_len) == 0; \
        ck_assert_msg(_assert_result, "Assertion 'memcmp("#VALUE", \"%s\", %zu)' failed", _actual_value, _expected_len); \
    } while(0)

