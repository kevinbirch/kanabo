#include <errno.h>

#include "conditions.h"
#include "evaluator.h"

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

static void freedom_iterator(void *each)
{
    dispose_node(node(each));
}

void nodelist_destroy(Nodelist *self)
{
    vector_destroy(self, freedom_iterator);
}

static bool nodelist_iterator_adpater(void *each, void *context);
static bool nodelist_map_adpater(void *each, void *context, Vector *target);

static bool nodelist_iterator_adpater(void *each, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.foreach(node(each), adapter->context);
}

bool nodelist_iterate(const Nodelist *list, nodelist_iterator iterator, void *context)
{
    ENSURE_NONNULL_ELSE_FALSE(list, iterator);
    return vector_iterate(list, nodelist_iterator_adpater, &(context_adapter){.iterator.foreach=iterator, context});
}

static bool nodelist_map_adpater(void *each, void *context, Vector *target)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator.map(node(each), adapter->context, target);
}

Nodelist *nodelist_map(const Nodelist *list, nodelist_map_function function, void *context)
{
    ENSURE_NONNULL_ELSE_NULL(list, function);

    Nodelist *target = make_nodelist_with_capacity(nodelist_length(list));
    context_adapter ctx = {.iterator.map=function, .context=context};

    if(!vector_map_into(list, nodelist_map_adpater, &ctx, target))
    {
        dispose_nodelist(target);
        target = NULL;
    }

    return target;
}
