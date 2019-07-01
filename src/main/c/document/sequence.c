#include "conditions.h"
#include "document.h"
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
    dispose_vector(self->values);
    self->values = NULL;
}

static String *sequence_repr(const Node *value)
{
    Sequence *self = (Sequence *)value;
    size_t len = vector_length(self->values);
    size_t line = self->position.line;
    size_t offset = self->position.offset;

    return format("<Sequence len: %zd, depth: %zd, pos: %zd:%zd>", len, self->depth, line, offset);
}

static bool seq_dumper(Node *each, void *context)
{
    node_dump(each, true);

    return true;
}

static void sequence_dump(const Node *value, bool pad)
{
    int padding = pad ? ((int)value->depth + 1) * INDENT : 0;
    String *repr = sequence_repr(value);
    fprintf(stdout, "%*c%s\n", padding, ' ', C(repr));
    dispose_string(repr);

    sequence_iterate(sequence(value), seq_dumper, NULL);
}

static const struct vtable_s sequence_vtable = 
{
    sequence_free,
    sequence_size,
    sequence_equals,
    sequence_repr,
    sequence_dump
};

Sequence *make_sequence_node(void)
{
    Sequence *self = xcalloc(sizeof(Sequence));
    node_init(&self->base, SEQUENCE, &sequence_vtable);
    self->values = make_vector();

    return self;
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

void sequence_add(Sequence *self, Node *item)
{
    ENSURE_NONNULL_ELSE_VOID(self, item);

    vector_add(self->values, item);
    item->document = self->document;
    item->parent = node(self);
    item->depth = self->depth + 1;
}

