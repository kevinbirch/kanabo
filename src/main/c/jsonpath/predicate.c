#include "jsonpath.h"
#include "conditions.h"

static const char * const PREDICATE_KIND_NAMES[] =
{
    "wildcard predicate",
    "subscript predicate",
    "slice predicate",
    "join predicate"
};

bool step_has_predicate(const Step *value)
{
    PRECOND_NONNULL_ELSE_FALSE(value);
    return NULL != value->predicate;
}

Predicate *step_predicate(const Step *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    PRECOND_NONNULL_ELSE_NULL(value->predicate);
    return value->predicate;
}

enum predicate_kind predicate_kind(const Predicate *value)
{
    return value->kind;
}

const char *predicate_kind_name(enum predicate_kind value)
{
    return PREDICATE_KIND_NAMES[value];
}
