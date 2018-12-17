#pragma once

#include "nodelist.h"

// nodelist assertions
#define assert_nodelist_length(NODELIST, EXPECTED_LENGTH) assert_uint_eq(EXPECTED_LENGTH, nodelist_length(NODELIST))
#define assert_nodelist_empty(NODELIST) assert_true(nodelist_is_empty((NODELIST)))
