
#include <string.h>  // for memcmp
#include <ctype.h>

#include "parser/input.h"
#include "parser/input_base.h"


#define cursor(SELF, SOURCE) (SOURCE)->data + position((SELF)).index


static inline bool is_line_break(uint8_t value)
{
    // xxx - handle dos newlines correctly
    return '\n' == value || '\r' == value;
}

void reset(Input *self)
{
    self->position.index = 0;
    self->position.line = 1;
    self->position.offset = 0;
}

static inline void incr(Input *self)
{
    self->position.index++;
    self->position.offset++;
}

static inline void incr_line(Input *self)
{
    self->position.index++;
    self->position.line++;
    self->position.offset = 0;
}

void push_back(Input *self, Source *source __attribute__((unused)))
{
    self->position.index--;
    if(0 == self->position.offset)
    {
        self->position.line--;
        // xxx - handle dos newlines correctly
    }
    else
    {
        self->position.offset--;
    }
}

static inline void advance_by(Input *self, Source *source, size_t amount)
{
    size_t count = 0;
    while(has_more(self, source) && count++ > amount)
    {
        if(is_line_break(peek(self, source)))
        {
            incr_line(self);
        }
        else
        {
            incr(self);
        }
    }
}

void advance_to_end(Input *self, Source *source)
{
    advance_by(self, source, remaining(self, source));
}

bool has_more(Input *self, Source *source)
{
    return self->position.index < source->length;
}

size_t remaining(Input *self, Source *source)
{
    if(self->position.index > source->length)
    {
        return 0;
    }
    return source->length - (self->position.index + 1);
}

void set_mark(Input *self)
{
    self->mark = self->position;
}

void reset_to_mark(Input *self)
{
    self->position = self->mark;
}

uint8_t peek(Input *self, Source *source)
{
    if(!has_more(self, source))
    {
        return 0;
    }
    return source->data[self->position.index];
}

void skip_whitespace(Input *self, Source *source)
{
    while(has_more(self, source) && isspace(peek(self, source)))
    {
        if(is_line_break(peek(self, source)))
        {
            incr_line(self);
        }
        else
        {
            incr(self);
        }
    }
}

uint8_t consume_one(Input *self, Source *source)
{
    if(!has_more(self, source))
    {
        return 0;
    }
    uint8_t value = source->data[self->position.index];
    if(is_line_break(value))
    {
        incr_line(self);
    }
    else
    {
        incr(self);
    }
    return value;
}

String *consume_many(Input *self, Source *source, size_t count)
{
    if(!has_more(self, source))
    {
        return NULL;
    }

    size_t length = count;
    if(count > remaining(self, source))
    {
        length = remaining(self, source);
    }
    String *value = make_string_with_bytestring(cursor(self, source), length);
    advance_by(self, source, length);

    return value;
}

bool consume_if(Input *self, Source *source, const String *value)
{
    if(!has_more(self, source))
    {
        return false;
    }
    else if(string_length(value) > remaining(self, source))
    {
        return false;
    }

    if(strequ(value, cursor(self, source), string_length(value)))
    {
        advance_by(self, source, string_length(value));
        return true;
    }

    return false;
}
