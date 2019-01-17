#include "conditions.h"
#include "document.h"
#include "xalloc.h"

static void alias_free(Node *value)
{
    // this space intentionally left blank
}

static bool alias_equals(const Node *one, const Node *two)
{
    return node_equals(const_alias(one)->target,
                       const_alias(two)->target);
}

static size_t alias_size(const Node *self)
{
    return 0;
}

static const struct vtable_s alias_vtable = 
{
    alias_free,
    alias_size,
    alias_equals
};

Alias *make_alias_node(Node *target)
{
    Alias *self = xcalloc(sizeof(Alias));
    node_init(node(self), ALIAS, &alias_vtable);
    self->target = target;

    return self;
}
