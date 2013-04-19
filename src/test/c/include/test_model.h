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

#include "model.h"

// model assertions
#define assert_node_kind(NODE, EXPECTED_KIND) assert_int_eq((EXPECTED_KIND), node_get_kind((NODE)))
#define assert_node_size(NODE, EXPECTED_SIZE) assert_int_eq((EXPECTED_SIZE), node_get_size((NODE)))
#define assert_mapping_has_key(NODE, KEY)     assert_true(mapping_contains_key((NODE), (KEY)))
#define assert_mapping_has_no_key(NODE, KEY)  assert_false(mapping_contains_key((NODE), (KEY)))
#define assert_scalar_value(NODE, VALUE) do {                           \
        assert_not_null(NODE);                                          \
        assert_node_kind(NODE, SCALAR);                                 \
        char *_assert_value = (VALUE);                                  \
        size_t _assert_length = strlen(_assert_value);                  \
        node *_assert_node = (NODE);                                    \
        size_t _assert_node_size = node_get_size(_assert_node);         \
        char _assert_scalar_value[_assert_node_size + 1];               \
        memcpy(&_assert_scalar_value, scalar_get_value(_assert_node), _assert_node_size); \
        _assert_scalar_value[_assert_node_size] = '\0';                 \
        ck_assert_msg(memcmp(_assert_value, _assert_scalar_value, _assert_length > _assert_node_size ? _assert_node_size : _assert_length) == 0, \
                      "Assertion 'memcmp("#VALUE", \"%s\", %zd)' failed", _assert_scalar_value,  _assert_node_size > _assert_length ? _assert_length : _assert_node_size); \
    } while(0);

