#include "jsonpath/model.h"

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

enum step_kind step_kind(const Step *value)
{
    return value->kind;
}

enum test_kind step_test_kind(const Step *value)
{
    return value->test.kind;
}

const char *step_kind_name(enum step_kind value)
{
    return STEP_KIND_NAMES[value];
}

const char *test_kind_name(enum test_kind value)
{
    return TEST_KIND_NAMES[value];
}
