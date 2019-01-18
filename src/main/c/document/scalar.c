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
    size_t n1 = node_size(one);
    size_t n2 = node_size(two);

    if(n1 != n2)
    {
        return false;
    }
    return 0 == memcmp(scalar_value((const Scalar *)one),
                       scalar_value((const Scalar *)two), n1);
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

static const struct vtable_s scalar_vtable = 
{
    scalar_free,
    scalar_size,
    scalar_equals
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
