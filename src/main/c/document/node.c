#include <stdio.h>
#include <string.h>

#include "conditions.h"
#include "document.h"
#include "log.h"
#include "xalloc.h"

static const char * const NODE_KINDS [] =
{
    "document",
    "scalar",
    "sequence",
    "mapping",
    "alias"
};

const char *node_kind_name_(const Node *self)
{
    return NODE_KINDS[self->tag.kind];
}

#define assert_kind(EXPECTED, ACTUAL, FILE, LINE) if((ACTUAL) != (EXPECTED)) \
    {                                                                   \
        printf("invalid cast from `%s` to `%s` at %s:%d\n",             \
               NODE_KINDS[(ACTUAL)], NODE_KINDS[(EXPECTED)], file, line); \
        exit(EXIT_FAILURE);                                             \
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

void _dispose_node(Node *value)
{
    if(NULL == value)
    {
        return;
    }
    value->vtable->free(value);
    free(value->tag.name);
    free(value->anchor);
    free(value);
}

size_t node_size_(const Node *self)
{
    ENSURE_NONNULL_ELSE_ZERO(self);

    return self->vtable->size(self);
}

NodeKind node_kind_(const Node *self)
{
    return self->tag.kind;
}

uint8_t *node_name_(const Node *self)
{
    ENSURE_NONNULL_ELSE_NULL(self);

    return self->tag.name;
}

Node *node_parent_(const Node *self)
{
    ENSURE_NONNULL_ELSE_NULL(self);
    return self->parent;
}

void node_set_tag_(Node *self, const uint8_t *value, size_t length)
{
    ENSURE_NONNULL_ELSE_VOID(self, value);
    self->tag.name = xcalloc(length + 1);
    memcpy(self->tag.name, value, length);
    self->tag.name[length] = '\0';
}

void node_set_anchor_(Node *self, const uint8_t *value, size_t length)
{
    ENSURE_NONNULL_ELSE_VOID(self, value);
    self->anchor = xcalloc(length + 1);
    memcpy(self->anchor, value, length);
    self->anchor[length] = '\0';
}

static bool tag_equals(const uint8_t *one, const uint8_t *two)
{
    if(NULL == one && NULL == two)
    {
        return true;
    }
    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }
    size_t n1 = strlen((char *)one);
    size_t n2 = strlen((char *)two);
    return memcmp(one, two, n1 > n2 ? n2 : n1) == 0;
}

bool node_equals_(const Node *one, const Node *two)
{
    if(one == two)
    {
        return true;
    }

    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }

    if(!(one->tag.kind == two->tag.kind &&
       tag_equals(one->tag.name, two->tag.name)))
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
