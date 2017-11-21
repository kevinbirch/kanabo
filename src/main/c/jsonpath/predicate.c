#include "jsonpath.h"
#include "conditions.h"

static const char * const PREDICATE_KIND_NAMES[] =
{
    "wildcard predicate",
    "subscript predicate",
    "slice predicate",
    "join predicate"
};

const char *predicate_kind_name(enum predicate_kind value)
{
    return PREDICATE_KIND_NAMES[value];
}
