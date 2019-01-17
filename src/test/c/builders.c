#include <stdarg.h>
#include <string.h>

#include "builders.h"

Node *sequence_builder(Node *one, ...)
{
    Sequence *sequence = make_sequence_node();
    sequence_add(sequence, one);

    va_list items;
    va_start(items, one);
    for(Node *each = va_arg(items, Node *); NULL != each; each = va_arg(items, Node *))
    {
        sequence_add(sequence, each);
    }
    va_end(items);

    return node(sequence);
}

Node *mapping_builder(const char *key1_repr, Node *value1, ...)
{
    Mapping *mapping = make_mapping_node();
    String *key_1 = make_string(key1_repr);
    mapping_put(mapping, key_1, value1);
    
    va_list values;
    va_start(values, value1);
    char *key_n_repr = va_arg(values, char *);
    while(NULL != key_n_repr)
    {
        String *key_n = make_string(key_n_repr);
        Node *value_n = va_arg(values, Node *);
        mapping_put(mapping, key_n, value_n);
    }
    va_end(values);

    return node(mapping);
}

Node *string(const char *value)
{
    return node(make_scalar_node(make_string(value), SCALAR_STRING));
}

Node *integer(const char *value)
{
    return node(make_scalar_node(make_string(value), SCALAR_INTEGER));
}

Node *real(const char *value)
{
    return node(make_scalar_node(make_string(value), SCALAR_REAL));
}

Node *timestamp(const char *value)
{
    return node(make_scalar_node(make_string(value), SCALAR_TIMESTAMP));
}

Node *boolean(bool value)
{
    char *scalar = value ? "true" : "false";
    return node(make_scalar_node(make_string(scalar), SCALAR_BOOLEAN));
}

Node *null(void)
{
    return node(make_scalar_node(make_string("null"), SCALAR_NULL));
}
