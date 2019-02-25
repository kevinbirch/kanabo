#include <stdio.h>
#include <string.h>

#include "conditions.h"
#include "document.h"
#include "log.h"
#include "panic.h"
#include "xalloc.h"

static const char * const NODE_KINDS [] =
{
    "document",
    "scalar",
    "sequence",
    "mapping",
    "alias"
};

const char *(node_kind_name)(const Node *self)
{
    return NODE_KINDS[self->tag.kind];
}

#define assert_kind(EXPECTED, ACTUAL, FILE, LINE)                       \
    if((ACTUAL) != (EXPECTED))                                          \
    {                                                                   \
        panicf("invalid cast from `%s` to `%s`",                        \
               NODE_KINDS[(ACTUAL)], NODE_KINDS[(EXPECTED)]);           \
    }

Node *node_narrow(Node *instance, NodeKind kind, const char * restrict file, int line)
{
    assert_kind(kind, instance->tag.kind, file, line);
    return instance;
}

const Node *const_node_narrow(const Node *instance, NodeKind kind, const char * restrict file, int line)
{
    assert_kind(kind, instance->tag.kind, file, line);
    return instance;
}

void node_init(Node *self, NodeKind kind, const struct vtable_s *vtable)
{
    if(NULL != self)
    {
        self->tag.kind = kind;
        self->tag.name = NULL;
        self->vtable = vtable;
        self->anchor = NULL;
    }
}

void (dispose_node)(Node *value)
{
    if(NULL == value)
    {
        return;
    }

    value->vtable->free(value);
    free(value->tag.name);
    dispose_string(value->anchor);

    free(value);
}

void (node_set_tag)(Node *self, String *value)
{
    ENSURE_NONNULL_ELSE_VOID(self, value);
    self->tag.name = value;
}

void (node_set_anchor)(Node *self, String *value)
{
    ENSURE_NONNULL_ELSE_VOID(self, value);

    self->anchor = value;
}

bool (node_equals)(const Node *one, const Node *two)
{
    if(one == two)
    {
        return true;
    }

    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }

    if(one->tag.kind != two->tag.kind)
    {
        return false;
    }
    if((NULL != one->tag.name || NULL != two->tag.name) &&
       !string_equals(one->tag.name, two->tag.name))
    {
        return false;
    }
    if(one->vtable->size(one) != two->vtable->size(two))
    {
        return false;
    }

    return one->vtable->equals(one, two);
}

bool node_comparitor(const void *one, const void *two)
{
    return node_equals(one, two);
}
