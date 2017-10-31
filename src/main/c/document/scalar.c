#include <string.h>

#include "document.h"
#include "conditions.h"


uint8_t *scalar_value(const node *scalar)
{
    PRECOND_NONNULL_ELSE_NULL(scalar);
    PRECOND_ELSE_NULL(SCALAR == node_kind(scalar));

    return scalar->content.scalar.value;
}

enum scalar_kind scalar_kind(const node *scalar)
{
    return scalar->content.scalar.kind;
}

bool scalar_boolean_is_true(const node *scalar)
{
    return 0 == memcmp("true", scalar_value(scalar), 4);
}

bool scalar_boolean_is_false(const node *scalar)
{
    return 0 == memcmp("false", scalar_value(scalar), 5);
}
