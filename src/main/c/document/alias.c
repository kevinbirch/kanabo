#include "document.h"
#include "conditions.h"

extern void node_init(Node *value, NodeKind kind);
extern bool node_comparitor(const void *one, const void *two);

static void alias_free(Node *value)
{
    // this space intentionally left blank
}

static bool alias_equals(const Node *one, const Node *two)
{
    return node_equals(alias_target((const Alias *)one),
                       alias_target((const Alias *)two));
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
    Alias *self = calloc(1, sizeof(Alias));
    if(NULL != self)
    {
        node_init(node(self), ALIAS);
        self->target = target;
        self->base.vtable = &alias_vtable;
    }

    return self;
}

Node *alias_target(const Alias *self)
{
    PRECOND_NONNULL_ELSE_NULL(self);

    return self->target;
}
