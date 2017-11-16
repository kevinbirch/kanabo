#include "jsonpath.h"
#include "conditions.h"

JsonPath *join_predicate_left(const Predicate *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_ELSE_NULL(JOIN == value->kind);

    return value->join.left;
}

JsonPath *join_predicate_right(const Predicate *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_ELSE_NULL(JOIN == value->kind);

    return value->join.right;
}
