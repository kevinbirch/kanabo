#include "jsonpath/model.h"
#include "conditions.h"

uint8_t *name_test_step_name(const Step *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_ELSE_NULL(NAME_TEST == value->test.kind);
    return value->test.name.value;
}

size_t name_test_step_length(const Step *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(NAME_TEST == value->test.kind);
    return value->test.name.length;
}
