#include "conditions.h"
#include "jsonpath.h"

static const char * const TYPE_TEST_KIND_NAMES[] =
{
    "object test",
    "array test",
    "string test",
    "number test",
    "boolean test",
    "null test"
};

const char *type_test_kind_name(enum type_test_kind value)
{
    return TYPE_TEST_KIND_NAMES[value];
}
