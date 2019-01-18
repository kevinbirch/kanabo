#pragma once

#include "document.h"

// model assertions
#define assert_node_kind(VALUE, EXPECTED) assert_int_eq((EXPECTED), node_kind(node((VALUE))))
#define assert_node_size(NODE, EXPECTED)  assert_uint_eq((EXPECTED), node_size(node((NODE))))

#define assert_node_tag(NODE, TAG) do {                                 \
        assert_not_null(NODE);                                          \
        char    *_expected_tag = (TAG);                                 \
        size_t   _expected_len = strlen(_expected_tag);                 \
        Node    *_assert_node = (NODE);                                 \
        uint8_t *_assert_name = node_name(_assert_node);                \
        size_t   _assert_name_len = strlen((char *)_assert_name);       \
        assert_uint_eq(_expected_len, _assert_name_len);                \
        char     _actual_tag[_assert_name_len + 1];                     \
        memcpy(&_actual_tag, _assert_name, _assert_name_len);           \
        _actual_tag[_assert_name_len] = '\0';                           \
        bool _assert_result = memcmp(_expected_tag, _actual_tag, _expected_len) == 0; \
        ck_assert_msg(_assert_result, "Assertion 'memcmp("#TAG", \"%s\", %zu)' failed", _actual_tag, _expected_len); \
    } while(0)

#define assert_node_equals(X, Y) assert_true(node_equals((X), (Y)))

#define assert_mapping_has_key(NODE, KEY) do {                          \
        String *key = make_string((KEY));                               \
        assert_true(mapping_contains((NODE), key));                     \
        dispose_string(key);                                               \
    } while(0)

#define assert_scalar_kind(NODE, EXPECTED) assert_int_eq(EXPECTED, scalar_kind(scalar((NODE))))

#define assert_scalar_value(NODE, VALUE) do {                           \
        assert_not_null(NODE);                                          \
        assert_node_kind(NODE, SCALAR);                                 \
        assert_not_null(VALUE);                                         \
        ck_assert_str_eq((VALUE), C(scalar_value(scalar((NODE)))));     \
    } while(0)

