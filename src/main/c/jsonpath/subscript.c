#include "jsonpath.h"
#include "conditions.h"

size_t subscript_predicate_index(const Predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SUBSCRIPT == value->kind);
    return value->subscript.index;
}
