#include "jsonpath.h"

static const char * const STEP_KIND_NAMES[] =
{
    "root step",
    "single step",
    "recursive step"
};

static const char * const TEST_KIND_NAMES[] =
{
    "wildcard test",
    "name test",
    "type test"
};

const char *step_kind_name(enum step_kind value)
{
    return STEP_KIND_NAMES[value];
}

const char *test_kind_name(enum test_kind value)
{
    return TEST_KIND_NAMES[value];
}
