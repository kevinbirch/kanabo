#include "document.h"
#include "conditions.h"

static bool mapping_iterator_adpater(void *key, void *value, void *context);

struct context_adapter_s
{
    mapping_iterator mapping;
    void *context;
};

typedef struct context_adapter_s context_adapter;

node *mapping_get(const node *mapping, uint8_t *scalar, size_t length)
{
    PRECOND_NONNULL_ELSE_NULL(mapping, scalar);
    PRECOND_ELSE_NULL(MAPPING == node_kind(mapping));
    PRECOND_ELSE_NULL(0 < length);

    node *key = make_scalar_node(scalar, length, SCALAR_STRING);
    node *result = hashtable_get(mapping->content.mapping, key);
    node_free(key);
    
    return result;
}

bool mapping_contains(const node *mapping, uint8_t *scalar, size_t length)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, scalar);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping), 0 < length);

    node *key = make_scalar_node(scalar, length, SCALAR_STRING);
    bool result = hashtable_contains(mapping->content.mapping, key);
    node_free(key);
    
    return result;
}

static bool mapping_iterator_adpater(void *key, void *value, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->mapping((node *)key, (node *)value, adapter->context);
}

bool mapping_iterate(const node *mapping, mapping_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, iterator);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping));

    context_adapter adapter = {.mapping=iterator, .context=context};
    return hashtable_iterate(mapping->content.mapping, mapping_iterator_adpater, &adapter);
}

bool mapping_put(node *mapping, uint8_t *scalar, size_t length, node *value)
{
    PRECOND_NONNULL_ELSE_FALSE(mapping, scalar, value);
    PRECOND_ELSE_FALSE(MAPPING == node_kind(mapping));

    node *key = make_scalar_node(scalar, length, SCALAR_STRING);
    if(NULL == key)
    {
        return false;
    }
    errno = 0;
    hashtable_put(mapping->content.mapping, key, value);
    if(0 == errno)
    {
        mapping->content.size = hashtable_size(mapping->content.mapping);
        value->parent = mapping;
    }
    return 0 == errno;
}

