#include "jsonpath/model.h"
#include "conditions.h"

static const char * const TYPE_TEST_KIND_NAMES[] =
{
    "object test",
    "array test",
    "string test",
    "number test",
    "boolean test",
    "null test"
};

enum type_test_kind type_test_step_kind(const Step *value)
{
    return value->test.type;
}

const char *type_test_kind_name(enum type_test_kind value)
{
    return TYPE_TEST_KIND_NAMES[value];
}
