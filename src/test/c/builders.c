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
    Scalar *key1 = make_scalar_node((uint8_t *)key1_repr, strlen(key1_repr), SCALAR_STRING);
    mapping_put(mapping, key1, value1);
    
    va_list values;
    va_start(values, value1);
    char *key_n_repr = va_arg(values, char *);
    while(NULL != key_n_repr)
    {
        Scalar *key_n = make_scalar_node((uint8_t *)key_n_repr, strlen(key_n_repr), SCALAR_STRING);
        Node *value_n = va_arg(values, Node *);
        mapping_put(mapping, key_n, value_n);
    }
    va_end(values);

    return node(mapping);
}

Node *string(const char *value)
{
    return node(make_scalar_node((uint8_t *)value, strlen(value), SCALAR_STRING));
}

Node *integer(const char *value)
{
    return node(make_scalar_node((uint8_t *)value, strlen(value), SCALAR_INTEGER));
}

Node *real(const char *value)
{
    return node(make_scalar_node((uint8_t *)value, strlen(value), SCALAR_REAL));
}

Node *timestamp(const char *value)
{
    return node(make_scalar_node((uint8_t *)value, strlen(value), SCALAR_TIMESTAMP));
}

Node *boolean(bool value)
{
    char *scalar = value ? "true" : "false";
    size_t len = value ? 4ul : 5ul;
    return node(make_scalar_node((uint8_t *)scalar, len, SCALAR_BOOLEAN));
}

Node *null(void)
{
    return node(make_scalar_node((uint8_t *)"null", 4ul, SCALAR_NULL));
}
