#include "document.h"
#include "conditions.h"

node *alias_target(const node *alias)
{
    PRECOND_NONNULL_ELSE_NULL(alias);
    PRECOND_ELSE_NULL(ALIAS == node_kind(alias));

    return alias->content.target;
}
