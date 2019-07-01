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

static String *alias_repr(const Node *value)
{
    Alias *self = (Alias *)value;
    size_t line = self->position.line;
    size_t offset = self->position.offset;

    const char *name = "NULL";
    if(NULL != self->target)
    {
        name = node_kind_name(self->target);
    }

    const char *anchor = NULL == self->base_yaml.anchor ? "NULL" : C(self->base_yaml.anchor);

    return format(
        "<Alias target: %s, anchor: %s, depth: %zu, pos: %zu:%zu>",
        name, anchor, self->depth, line, offset);
}

static void alias_dump(const Node *value, bool pad)
{
    int padding = pad ? ((int)value->depth + 1) * INDENT : 0;
    String *repr = alias_repr(value);
    fprintf(stdout, "%*c%s\n", padding, ' ', C(repr));
    dispose_string(repr);

    repr = node_repr(alias(value)->target);
    fprintf(stdout, "%*c%s\n", padding + INDENT, ' ', C(repr));
    dispose_string(repr);
}

static const struct vtable_s alias_vtable = 
{
    alias_free,
    alias_size,
    alias_equals,
    alias_repr,
    alias_dump
};

Alias *make_alias_node(Node *target)
{
    Alias *self = xcalloc(sizeof(Alias));
    node_init(node(self), ALIAS, &alias_vtable);
    self->target = target;

    return self;
}
