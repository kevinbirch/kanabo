#include "document.h"
#include "conditions.h"

struct context_adapter_s
{
    sequence_iterator sequence;
    void *context;
};

typedef struct context_adapter_s context_adapter;

static bool sequence_iterator_adpater(void *each, void *context);

node *sequence_get(const node *sequence, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(sequence);
    PRECOND_ELSE_NULL(SEQUENCE == node_kind(sequence));
    PRECOND_ELSE_NULL(index < node_size(sequence));

    return vector_get(sequence->content.sequence, index);
}

static bool sequence_iterator_adpater(void *each, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->sequence((node *)each, adapter->context);
}

bool sequence_iterate(const node *sequence, sequence_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(sequence, iterator);
    PRECOND_ELSE_FALSE(SEQUENCE == node_kind(sequence));

    context_adapter adapter = {.sequence=iterator, .context=context };
    return vector_iterate(sequence->content.sequence, sequence_iterator_adpater, &adapter);
}

bool sequence_add(node *sequence, node *item)
{
    PRECOND_NONNULL_ELSE_FALSE(sequence, item);
    PRECOND_ELSE_FALSE(SEQUENCE == node_kind(sequence));

    bool result = vector_add(sequence->content.sequence, item);
    if(result)
    {
        sequence->content.size = vector_length(sequence->content.sequence);
        item->parent = sequence;
    }
    return result;
}

