#include <stdarg.h>
#include <string.h>

#include "builders.h"

node *sequence_builder(node *one, ...)
{
    node *sequence = make_sequence_node();
    sequence_add(sequence, one);

    va_list items;
    va_start(items, one);
    for(node *each = va_arg(items, node *); NULL != each; each = va_arg(items, node *))
    {
        sequence_add(sequence, each);
    }
    va_end(items);

    return sequence;
}

node *mapping_builder(const char *key1, node *value1, ...)
{
    node *mapping = make_mapping_node();
    mapping_put(mapping, (uint8_t *)key1, strlen(key1), value1);
    
    va_list values;
    va_start(values, value1);
    char *key = va_arg(values, char *);
    while(NULL != key)
    {
        node *value = va_arg(values, node *);
        mapping_put(mapping, (uint8_t *)key, strlen(key), value);
    }
    va_end(values);

    return mapping;
}

node *string(const char *value)
{
    return make_scalar_node((uint8_t *)value, strlen(value), SCALAR_STRING);
}

node *integer(int value)
{
    char *scalar;
    asprintf(&scalar, "%i", value);
    node *result = make_scalar_node((uint8_t *)scalar, strlen(scalar), SCALAR_INTEGER);
    free(scalar);
    return result;
}

node *real(float value)
{
    char *scalar;
    asprintf(&scalar, "%f", value);
    node *result = make_scalar_node((uint8_t *)scalar, strlen(scalar), SCALAR_REAL);
    free(scalar);
    return result;
}

node *timestamp(const char *value)
{
    return make_scalar_node((uint8_t *)value, strlen(value), SCALAR_TIMESTAMP);
}

node *boolean(bool value)
{
    char *scalar = value ? "true" : "false";
    return make_scalar_node((uint8_t *)scalar, strlen(scalar), SCALAR_BOOLEAN);
}

node *null(void)
{
    return make_scalar_node((uint8_t *)"null", 4ul, SCALAR_NULL);
}
