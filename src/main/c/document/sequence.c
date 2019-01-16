#include "conditions.h"
#include "document.h"
#include "panic.h"
#include "xalloc.h"

struct context_adapter_s
{
    sequence_iterator iterator;
    void *context;
};

typedef struct context_adapter_s context_adapter;

static bool sequence_equals(const Node *one, const Node *two)
{
    return vector_equals(const_sequence(one)->values,
                         const_sequence(two)->values,
                         node_comparitor);
}

static size_t sequence_size(const Node *self)
{
    return vector_length(const_sequence(self)->values);
}

static bool sequence_freedom_iterator(void *each, void *context __attribute__((unused)))
{
    dispose_node(each);

    return true;
}

static void sequence_free(Node *value)
{
    Sequence *self = (Sequence *)value;
    if(NULL == self->values)
    {
        return;
    }
    vector_iterate(self->values, sequence_freedom_iterator, NULL);
    vector_free(self->values);
    self->values = NULL;
}

static const struct vtable_s sequence_vtable = 
{
    sequence_free,
    sequence_size,
    sequence_equals
};

Sequence *make_sequence_node(void)
{
    Sequence *self = xcalloc(sizeof(Sequence));
    node_init(&self->base, SEQUENCE, &sequence_vtable);
    self->values = make_vector();
    if(NULL == self->values)
    {
        panic("unable to allocate sequence vector delegate");
    }

    return self;
}

Node *sequence_get(const Sequence *self, size_t index)
{
    ENSURE_NONNULL_ELSE_NULL(self);
    ENSURE_ELSE_NULL(index < vector_length(self->values));

    return vector_get(self->values, index);
}

static bool sequence_iterator_adpater(void *each, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator(node(each), adapter->context);
}

bool sequence_iterate(const Sequence *self, sequence_iterator iterator, void *context)
{
    ENSURE_NONNULL_ELSE_FALSE(self, iterator);

    context_adapter adapter = {.iterator=iterator, .context=context};
    return vector_iterate(self->values, sequence_iterator_adpater, &adapter);
}

bool sequence_add(Sequence *self, Node *item)
{
    ENSURE_NONNULL_ELSE_FALSE(self, item);

    bool result = vector_add(self->values, item);
    if(result)
    {
        item->parent = node(self);
    }

    return result;
}

