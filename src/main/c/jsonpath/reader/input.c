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

#include <string.h>
#include <ctype.h>

#include "jsonpath/input.h"


struct input_s
{
    const uint8_t *data;
    size_t         length;
    size_t         cursor;
    size_t         mark;
};


void reset(Input *input)
{
    input->cursor = 0;
}

void set_mark(Input *input)
{
    input->mark = input->cursor;
}

void reset_to_mark(Input *input)
{
    input->cursor = input->mark;
}

bool has_more(Input *input)
{
    return input->cursor < input->length;
}

size_t remaining(Input *input)
{
    return input->length - input->cursor;
}

void skip_whitespace(Input *input)
{
    if(!has_more(input))
    {
        return;
    }
    while(isspace(peek(input)))
    {
        consume_char(input);
    }
}

uint8_t peek(Input *input)
{
    if(!has_more(input))
    {
        return 0;
    }
    return input->data[input->cursor];
}

uint8_t consume_char(Input *input)
{
    if(!has_more(input))
    {
        return 0;
    }
    return input->data[input->cursor++];
}

void consume_many(Input *input, size_t count)
{
    if(!has_more(input))
    {
        return;
    }
    else if(count > remaining(input))
    {
        input->cursor = input->length - 1;
    }
    else
    {
        input->cursor += count;
    }
}

bool consume_if(Input *input, const char *value)
{
    if(!has_more(input))
    {
        return false;
    }

    size_t length = strlen(value);
    if(0 == memcmp(input->data, value, length))
    {
        consume_many(input, length);
        return true;
    }
    return false;
}

void push_back(Input *input)
{
    input->cursor--;
}

bool looking_at(Input *input, const char *value)
{
    if(!has_more(input))
    {
        return false;
    }
    size_t length = strlen(value);
    if(0 == memcmp(input->data, value, length))
    {
        return true;
    }
    return false;

}

size_t find(Input *input, const char *value)
{
    if(!has_more(input))
    {
        return 0;
    }
    return strcspn((char *)input->data + input->cursor, value);

}
