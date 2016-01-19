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


size_t cursor(Input *self)
{
    return self->cursor;
}

void reset(Input *self)
{
    self->cursor = 0;
}

void set_mark(Input *self)
{
    self->mark = self->cursor;
}

void reset_to_mark(Input *self)
{
    self->cursor = self->mark;
}

bool has_more(Input *self)
{
    return self->cursor < self->length;
}

size_t remaining(Input *self)
{
    return self->length - self->cursor;
}

void skip_whitespace(Input *self)
{
    if(!has_more(self))
    {
        return;
    }
    while(isspace(peek(self)))
    {
        consume_char(self);
    }
}

uint8_t peek(Input *self)
{
    if(!has_more(self))
    {
        return 0;
    }
    return self->data[self->cursor];
}

uint8_t consume_char(Input *self)
{
    if(!has_more(self))
    {
        return 0;
    }
    return self->data[self->cursor++];
}

void consume_many(Input *self, size_t count)
{
    if(!has_more(self))
    {
        return;
    }
    else if(count > remaining(self))
    {
        self->cursor = self->length - 1;
    }
    else
    {
        self->cursor += count;
    }
}

bool consume_if(Input *self, const char *value)
{
    if(!has_more(self))
    {
        return false;
    }

    size_t length = strlen(value);
    if(0 == memcmp(self->data, value, length))
    {
        consume_many(self, length);
        return true;
    }
    return false;
}

void push_back(Input *self)
{
    self->cursor--;
}

bool looking_at(Input *self, const char *value)
{
    if(!has_more(self))
    {
        return false;
    }
    size_t length = strlen(value);
    if(0 == memcmp(self->data, value, length))
    {
        return true;
    }
    return false;

}

size_t find(Input *self, const char *value)
{
    if(!has_more(self))
    {
        return 0;
    }
    return strcspn((char *)self->data + self->cursor, value);

}
