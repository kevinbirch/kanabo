#include <string.h>

#include "document.h"
#include "conditions.h"

static bool node_comparitor(const void *one, const void *two);
static bool tag_equals(const uint8_t *one, const uint8_t *two);

static bool scalar_equals(const node *one, const node *two);
static bool sequence_equals(const node *one, const node *two);
static bool mapping_equals(const node *one, const node *two);


enum node_kind node_kind(const node *value)
{
    return value->tag.kind;
}

uint8_t *node_name(const node *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);

    return value->tag.name;
}

size_t node_size(const node *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);

    return value->content.size;
}

node *node_parent(const node *value)
{
    PRECOND_NONNULL_ELSE_NULL(value);
    return value->parent;
}

void node_set_tag(node *target, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->tag.name = (uint8_t *)calloc(1, length + 1);
    if(NULL != target->tag.name)
    {
        memcpy(target->tag.name, value, length);
        target->tag.name[length] = '\0';
    }
}

void node_set_anchor(node *target, const uint8_t *value, size_t length)
{
    PRECOND_NONNULL_ELSE_VOID(target, value);
    target->anchor = (uint8_t *)calloc(1, length + 1);
    if(NULL != target->anchor)
    {
        memcpy(target->anchor, value, length);
        target->anchor[length] = '\0';
    }
}

bool node_equals(const node *one, const node *two)
{
    if(one == two)
    {
        return true;
    }
    
    if((NULL == one && NULL != two) || (NULL != one && NULL == two))
    {
        return false;
    }

    bool result = node_kind(one) == node_kind(two) &&
        tag_equals(node_name(one), node_name(two)) &&
        node_size(one) == node_size(two);

    if(!result)
    {
        return result;
    }
    switch(node_kind(one))
    {
        case DOCUMENT:
            result &= node_equals(document_root(one), document_root(two));
            break;
        case SCALAR:
            result &= scalar_equals(one, two);
            break;
        case SEQUENCE:
            result &= sequence_equals(one, two);
            break;
        case MAPPING:
            result &= mapping_equals(one, two);
            break;
        case ALIAS:
            result &= node_equals(alias_target(one), alias_target(two));
            break;
    }
    return result;
}

static bool scalar_equals(const node *one, const node *two)
{
    size_t n1 = node_size(one);
    size_t n2 = node_size(two);

    if(n1 != n2)
    {
        return false;
    }
    return memcmp(scalar_value(one), scalar_value(two), n1) == 0;
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

static bool node_comparitor(const void *one, const void *two)
{
    return node_equals((node *)one, (node *)two);
}

static bool sequence_equals(const node *one, const node *two)
{
    return vector_equals(one->content.sequence, two->content.sequence, node_comparitor);
}

static bool mapping_equals(const node *one, const node *two)
{
    return hashtable_equals(one->content.mapping, two->content.mapping, node_comparitor);
}
