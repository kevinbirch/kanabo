#include <string.h>
#include <errno.h>

#include "document.h"
#include "conditions.h"

static inline node *make_node(enum node_kind kind);

static bool scalar_comparitor(const void *one, const void *two);
static hashcode scalar_hash(const void *key);

document_model *make_model(void)
{
    document_model *result = (document_model *)calloc(1, sizeof(document_model));
    if(NULL != result)
    {
        result->documents = make_vector();
        if(NULL == result->documents)
        {
            free(result);
            result = NULL;
            return NULL;
        }
    }

    return result;
}

node *make_document_node(void)
{
    node *result = make_node(DOCUMENT);
    if(NULL != result)
    {
        result->content.size = 0;
    }

    return result;
}

node *make_sequence_node(void)
{
    node *result = make_node(SEQUENCE);
    if(NULL != result)
    {
        result->content.sequence = make_vector();
        if(NULL == result->content.sequence)
        {
            free(result);
            result = NULL;
            return NULL;
        }
    }

    return result;
}    

node *make_mapping_node(void)
{
    node *result = make_node(MAPPING);
    if(NULL != result)
    {
        result->content.mapping = make_hashtable_with_function(scalar_comparitor, scalar_hash);
        if(NULL == result->content.mapping)
        {
            free(result);
            result = NULL;
            return NULL;
        }        
        result->content.size = 0;
    }

    return result;
}    

node *make_scalar_node(const uint8_t *value, size_t length, enum scalar_kind kind)
{
    if(NULL == value && 0 != length)
    {
        errno = EINVAL;
        return NULL;
    }

    node *result = make_node(SCALAR);
    if(NULL != result)
    {
        result->content.size = length;
        result->content.scalar.kind = kind;
        if(NULL != value)
        {
            result->content.scalar.value = (uint8_t *)calloc(1, length);
            if(NULL == result->content.scalar.value)
            {
                free(result);
                result = NULL;
                return NULL;
            }
            memcpy(result->content.scalar.value, value, length);
        }
    }

    return result;
}

node *make_alias_node(node *target)
{
    node *result = make_node(ALIAS);
    if(NULL != result)
    {
        result->content.size = 0;
        result->content.target = target;
    }

    return result;
}

static inline node *make_node(enum node_kind kind)
{
    node *result = (node *)calloc(1, sizeof(struct node));
    if(NULL != result)
    {
        result->tag.kind = kind;
        result->tag.name = NULL;
        result->anchor = NULL;
    }

    return result;
}

static hashcode scalar_hash(const void *key)
{
    node *scalar = (node *)key;
    return fnv1a_string_buffer_hash(scalar_value(scalar), node_size(scalar));
}

static bool scalar_comparitor(const void *one, const void *two)
{
    return node_equals((node *)one, (node *)two);
}
