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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/stat.h>

#include "parser/input.h"
#include "parser/source.h"


struct file_input_s
{
    union
    {
        struct input_s;
        Input base;
    };
    String *filename;
    Source  source;
};

struct buffer_input_s
{
    union
    {
        struct input_s;
        Input base;
    };
    Source source;
};


#define cursor(SELF) (SELF)->source.data + (SELF)->position.offset


static off_t file_size(FILE *file)
{
    struct stat stats;
    if(fstat(fileno(file), &stats))
    {
        return -1;
    }

    return stats.st_size;
}

static inline FileInput *file_input_alloc(size_t bufsize)
{
    return calloc(1, sizeof(FileInput) + bufsize);
}

static inline void file_input_init(FileInput *self, const char *filename, FILE *file, size_t size)
{
    self->base.position.line = 1;
    self->filename = make_string(filename);
    size_t count = fread(self->source.data, 1, size, file);
    self->source.length = count;
}

FileInput *make_file_input(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if(NULL == file)
    {
        return NULL;
    }
    off_t size = file_size(file);
    if(0 > size)
    {
        return NULL;
    }
    
    FileInput *self = file_input_alloc((size_t)size);
    if(NULL == self)
    {
        fclose(file);
        return NULL;
    }
    file_input_init(self, filename, file, (size_t)size);

    return self;
}

static inline BufferInput *buffer_input_alloc(size_t length)
{
    return calloc(1, sizeof(BufferInput) + length);
}

static inline void buffer_input_init(BufferInput *self, const uint8_t *data, size_t length)
{
    memcpy(self->source.data, data, length);
    self->source.length = length;
}

BufferInput *make_buffer_input(const uint8_t *data, size_t length)
{
    BufferInput *self = buffer_input_alloc(length);
    if(NULL == self)
    {
        return NULL;
    }
    buffer_input_init(self, data, length);

    return self;
}

void dispose_file_input(FileInput *self)
{
    string_free(self->filename);
    free(self);
}

void dispose_buffer_input(BufferInput *self)
{
    free(self);
}

String *file_input_name(FileInput *self)
{
    return self->filename;
}

Input *file_input_upcast(FileInput *self)
{
    return &self->base;
}

Input *buffer_input_upcast(BufferInput *self)
{
    return &self->base;
}

Postion input_position(const Input *self)
{
    return self->position;
}

void input_advance_to_end(Input *self)
{
    if(!has_more(self))
    {
        return;
    }
    self->position.offset = self->source.length - 1;
}

void rewind(Input *self)
{
    self->position.line = 1;
    self->position.offset = 0;
}

void set_mark(Input *self)
{
    self->mark = self->position;
}

void reset_to_mark(Input *self)
{
    self->position = self->mark;
}

bool has_more(Input *self)
{
    return self->position < self->length;
}

size_t remaining(Input *self)
{
    return self->length - self->position;
}

uint8_t peek(Input *self)
{
    if(!has_more(self))
    {
        return 0;
    }
    return self->data[self->position];
}

void skip_whitespace(Input *self)
{
    while(has_more(self) && isspace(peek(self)))
    {
        consume_char(self);
    }
}

uint8_t consume_one(Input *self)
{
    if(!has_more(self))
    {
        return 0;
    }
    return self->data[self->position++];
}

String *consume_many(Input *self, size_t count)
{
    if(!has_more(self))
    {
        return;
    }
    else if(count > remaining(self))
    {
        self->position = self->length - 1;
    }
    else
    {
        self->position += count;
    }
}

bool consume_if(Input *self, const uint8_t *value, size_t length)
{
    if(!has_more(self))
    {
        return false;
    }

    if(length > remaining(self))
    {
        return false;
    }
    if(0 == memcmp(cursor(self), value, length))
    {
        consume_many(self, length);
        return true;
    }

    return false;
}

void push_back(Input *self)
{
    self->position--;
}

bool looking_at(Input *self, const char *value)
{
    if(!has_more(self))
    {
        return false;
    }
    size_t length = strlen(value);
    if(length > remaining(self))
    {
        return false;
    }
    if(0 == memcmp(cursor(self), value, length))
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
    return strcspn((char *)cursor(self), value);

}
