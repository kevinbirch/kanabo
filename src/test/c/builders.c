/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 * 
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README file in the project root.
 *
 * Distributed under an [MIT-style][license] license.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimers in the documentation and/or
 *   other materials provided with the distribution.
 * - Neither the names of the copyright holders, nor the names of the authors, nor
 *   the names of other contributors may be used to endorse or promote products
 *   derived from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/ncsa
 */

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
