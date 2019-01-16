#include <errno.h>

#include "conditions.h"
#include "evaluator.h"
#include "panic.h"

struct context_adapter_s
{
    union
    {
        nodelist_iterator foreach;
        nodelist_map_function map;
    } iterator;
    void *context;
};

typedef struct context_adapter_s context_adapter;

static bool nodelist_iterator_adpater(void *each, void *context);
static bool nodelist_map_adpater(void *each, void *context, Vector *target);

void nodelist_add(Nodelist *list, Node *value)
{
    if(!vector_add(list, value))
    {
        panic("nodelist_add failed");
    }
}

void nodelist_set(Nodelist *list, Node *value, size_t index)
{
    errno = 0;
    vector_set(list, value, index);

    if(0 != errno)
    {
        panic("nodelist_set failed");
    }
}

static bool nodelist_iterator_adpater(void *each, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.foreach((Node *)each, adapter->context);
}

bool nodelist_iterate(const Nodelist *list, nodelist_iterator iterator, void *context)
{
    ENSURE_NONNULL_ELSE_FALSE(list, iterator);
    return vector_iterate(list, nodelist_iterator_adpater, &(context_adapter){.iterator.foreach=iterator, context});
}

static bool nodelist_map_adpater(void *each, void *context, Vector *target)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.map((Node *)each, adapter->context, target);
}

Nodelist *nodelist_map(const Nodelist *list, nodelist_map_function function, void *context)
{
    ENSURE_NONNULL_ELSE_NULL(list, function);
    return vector_map(list, nodelist_map_adpater, &(context_adapter){.iterator.map=function, context});
}

Nodelist *nodelist_map_into(const Nodelist *list, nodelist_map_function function, void *context, Nodelist *target)
{
    ENSURE_NONNULL_ELSE_NULL(list, function, target);
    return vector_map_into(list, nodelist_map_adpater, &(context_adapter){.iterator.map=function, context}, target);
}
