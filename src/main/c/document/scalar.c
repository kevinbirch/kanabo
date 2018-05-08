#include <string.h>

#include "document.h"
#include "conditions.h"

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
    return const_scalar(self)->length;
}

static void scalar_free(Node *value)
{
    Scalar *self = scalar(value);
    free(self->value);
    self->value = NULL;
}

static const struct vtable_s scalar_vtable = 
{
    scalar_free,
    scalar_size,
    scalar_equals
};

Scalar *make_scalar_node(const uint8_t *value, size_t length, ScalarKind kind)
{
    if(NULL == value && 0 != length)
    {
        errno = EINVAL;
        return NULL;
    }

    Scalar *self = xcalloc(sizeof(Scalar));
    node_init(node(self), SCALAR, &scalar_vtable);

    self->length = length;
    self->kind = kind;
    self->value = xcalloc(length);
    memcpy(self->value, value, length);

    return self;
}

uint8_t *scalar_value(const Scalar *self)
{
    PRECOND_NONNULL_ELSE_NULL(self);

    return self->value;
}

ScalarKind scalar_kind(const Scalar *self)
{
    return self->kind;
}

bool scalar_boolean_is_true(const Scalar *self)
{
    if(4 != scalar_size(node(self)))
    {
        return false;
    }
    return 0 == memcmp("true", self->value, 4);
}

bool scalar_boolean_is_false(const Scalar *self)
{
    if(5 != scalar_size(node(self)))
    {
        return false;
    }
    return 0 == memcmp("false", self->value, 5);
}
