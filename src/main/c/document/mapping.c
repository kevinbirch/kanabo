#include <errno.h>

#include "conditions.h"
#include "document.h"
#include "xalloc.h"

struct context_adapter_s
{
    mapping_iterator iterator;
    void *context;
};

typedef struct context_adapter_s context_adapter;

static bool mapping_equals(const Node *one, const Node *two)
{
    return hashtable_equals(((const Mapping *)one)->values,
                            ((const Mapping *)two)->values,
                            node_comparitor);
}

static size_t mapping_size(const Node *self)
{
    return hashtable_size(((const Mapping *)self)->values);
}

static bool mapping_freedom_iterator(void *key, void *value, void *context __attribute__((unused)))
{
    dispose_node(key);
    dispose_node(value);

    return true;
}

static void mapping_free(Node *value)
{
    Mapping *map = (Mapping *)value;
    if(NULL == map->values)
    {
        return;
    }

    hashtable_iterate(map->values, mapping_freedom_iterator, NULL);
    dispose_hashtable(map->values);
    map->values = NULL;
}

static String *mapping_repr(const Node *value)
{
    Mapping *self = (Mapping *)value;
    size_t len = hashtable_size(self->values);
    size_t line = self->position.line;
    size_t offset = self->position.offset;

    return format("<Mapping len: %zd, depth: %zd, pos: %zd:%zd>", len, self->depth, line, offset);
}

static bool map_dumper(String *key, Node *value, void *context)
{
    int padding = ((int)value->depth + 1) * INDENT;
    fprintf(stdout, "%*c\"%s\" -> ", padding, ' ', C(key));

    node_dump(value, false);

    return true;
}

static void mapping_dump(const Node *value, bool pad)
{
    int padding = pad ? ((int)value->depth + 1) * INDENT : 0;
    String *repr = node_repr(value);
    fprintf(stdout, "%*c%s\n", padding, ' ', C(repr));
    dispose_string(repr);

    mapping_iterate(mapping(value), map_dumper, NULL);
}

static const struct vtable_s mapping_vtable = 
{
    mapping_free,
    mapping_size,
    mapping_equals,
    mapping_repr,
    mapping_dump
};

static hashcode scalar_hash(const void *key)
{
    const String *value = scalar_value(key);
    return fnv1a_string_buffer_hash(strdta(value), strlen(value));
}

Mapping *make_mapping_node(void)
{
    Mapping *self = xcalloc(sizeof(Mapping));
    node_init(node(self), MAPPING, &mapping_vtable);
    self->values = make_hashtable_with_function(node_comparitor, scalar_hash);

    return self;
}

Node *mapping_get(const Mapping *self, String *value)
{
    ENSURE_NONNULL_ELSE_NULL(self, value);

    Scalar *key = make_scalar_node(value, SCALAR_STRING);

    Node *result = hashtable_get(self->values, key);

    key->value = NULL;
    dispose_node(key);

    return result;
}

bool mapping_contains(const Mapping *self, String *value)
{
    ENSURE_NONNULL_ELSE_FALSE(self, value);

    Scalar *key = make_scalar_node(value, SCALAR_STRING);

    bool result = hashtable_contains(self->values, key);

    key->value = NULL;
    dispose_node(key);

    return result;
}

static bool mapping_iterator_adpater(void *key, void *value, void *context)
{
    context_adapter *adapter = (context_adapter *)context;
    return adapter->iterator(scalar_value(key), node(value), adapter->context);
}

bool mapping_iterate(const Mapping *self, mapping_iterator iterator, void *context)
{
    ENSURE_NONNULL_ELSE_FALSE(self, iterator);

    context_adapter adapter = {.iterator=iterator, .context=context};
    return hashtable_iterate(self->values, mapping_iterator_adpater, &adapter);
}

bool mapping_put(Mapping *self, Scalar *key, Node *value)
{
    ENSURE_NONNULL_ELSE_FALSE(self, key, value);

    errno = 0;
    hashtable_put(self->values, key, value);
    if(0 == errno)
    {
        key->parent = node(self);
        value->parent = node(self);
        value->depth = self->depth + 1;
    }

    return 0 == errno;
}

