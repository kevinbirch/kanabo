#include <string.h>

#include "conditions.h"
#include "document.h"
#include "xalloc.h"

static const char * const SCALAR_KINDS [] =
{
    "string",
    "integer",
    "real",
    "timestamp",
    "boolean",
    "null"
};

const char *scalar_kind_name(const Scalar *self)
{
    return SCALAR_KINDS[scalar_kind(self)];
}

static bool scalar_equals(const Node *one, const Node *two)
{
    const Scalar *s1 = (const Scalar *)one;
    const Scalar *s2 = (const Scalar *)two;

    return string_equals(s1->value, s2->value);
}

static size_t scalar_size(const Node *self)
{
    return strlen(const_scalar(self)->value);
}

static void scalar_free(Node *value)
{
    Scalar *self = scalar(value);
    dispose_string(self->value);
    self->value = NULL;
}

static String *scalar_repr(const Node *value)
{
    Scalar *self = (Scalar *)value;
    size_t line = self->position.line;
    size_t offset = self->position.offset;
    const char *name = scalar_kind_name(self);
    const char *anchor = NULL == self->base_yaml.anchor ? "NULL" : C(self->base_yaml.anchor);

    return format(
        "<Scalar kind: %s, value: \"%s\", anchor: %s, depth: %zu, pos: %zu:%zu>",
        name, C(self->value), anchor, self->depth, line, offset);
}

static void scalar_dump(const Node *value, bool pad)
{
    int padding = pad ? ((int)value->depth + 1) * INDENT : 0;
    String *repr = scalar_repr(value);
    fprintf(stdout, "%*c%s\n", padding, ' ', C(repr));
    dispose_string(repr);
}

static const struct vtable_s scalar_vtable = 
{
    scalar_free,
    scalar_size,
    scalar_equals,
    scalar_repr,
    scalar_dump
};

Scalar *make_scalar_node(String *value, ScalarKind kind)
{
    ENSURE_NONNULL_ELSE_NULL(value);

    Scalar *self = xcalloc(sizeof(Scalar));
    node_init(node(self), SCALAR, &scalar_vtable);

    self->kind = kind;
    self->value = value;

    return self;
}
