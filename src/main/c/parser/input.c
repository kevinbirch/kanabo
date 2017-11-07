#include <ctype.h>
#include <stdlib.h>
#include <string.h>  // for memcmp, memcpy

#include "parser/input.h"

#define current(INPUT) (INPUT)->source.buffer[(INPUT)->position.index]
#define cursor(INPUT) (INPUT)->source.buffer + (INPUT)->position.index
#define index(INPUT) (INPUT)->position.index

static inline void incr(Input *self)
{
    self->position.index++;
    if(!self->track_lines)
    {
        return;
    }

    if(0x0A == current(self))
    {
        self->position.line++;
        self->position.offset = 0;
    }
    else
    {
        self->position.offset++;
    }
}

static inline void advance_by(Input *self, size_t amount)
{
    size_t count = 0;
    while(index(self) < self->source.length && count++ < amount)
    {
        incr(self);
    }
}

void input_init(Input *self, const char *name, size_t length)
{
    self->position.index = 0;
    self->position.line = 0;
    self->position.offset = 0;
    self->track_lines = false;

    if(NULL != name)
    {
        self->name = S(name);
    }
    self->source.length = length;
}

void input_release(Input *self)
{
    string_free(self->name);
}

void dispose_input(Input *self)
{
    string_free(self->name);
    free(self);
}

String *input_name(Input *self)
{
    return self->name;
}

size_t input_length(Input *self)
{
    return self->source.length;
}

void input_set_track_lines(Input *self, bool value)
{
    self->track_lines = value;
}

bool input_is_tracking_lines(Input *self)
{
    return self->track_lines;
}

Position input_position(const Input *self)
{
    return self->position;
}

void input_goto(Input *self, Position position)
{
    self->position = position;
}

void input_advance_to_end(Input *self)
{
    advance_by(self, input_remaining(self));
}

void input_reset(Input *self)
{
    self->position.index = 0;
    self->position.line = 0;
    self->position.offset = 0;
}

bool input_has_more(Input *self)
{
    return index(self) < self->source.length;
}

size_t input_remaining(Input *self)
{
    if(index(self) >= self->source.length)
    {
        return 0;
    }

    return self->source.length - index(self);
}

char input_peek(Input *self)
{
    if(!input_has_more(self))
    {
        return 0;
    }

    return current(self);
}

void input_skip_whitespace(Input *self)
{
    while(input_has_more(self) && isspace(current(self)))
    {
        incr(self);
    }
}

char input_consume_one(Input *self)
{
    if(!input_has_more(self))
    {
        return 0;
    }

    char current = current(self);
    incr(self);

    return current;
}

size_t input_consume_many(Input *self, size_t count, char *result)
{
    if(!input_has_more(self))
    {
        return 0;
    }

    size_t length = count;
    if(count > input_remaining(self))
    {
        length = input_remaining(self);
    }

    if(result != NULL)
    {
        memcpy(result, cursor(self), length);
        result[length] = '\0';
    }
    advance_by(self, length);

    return length;
}

bool input_consume_if(Input *self, const char *value)
{
    if(!input_has_more(self))
    {
        return false;
    }
    size_t length = strlen(value);
    if(length > input_remaining(self))
    {
        return false;
    }

    if(memcmp(cursor(self), value, length) == 0)
    {
        advance_by(self, length);
        return true;
    }

    return false;
}

void input_push_back(Input *self)
{
    if(0 == index(self))
    {
        return;
    }

    self->position.index--;
    if(!self->track_lines)
    {
        return;
    }

    if(0 == self->position.offset)
    {
        // NB - track back from the position preceeding the previous newline
        size_t i = index(self) - 1;
        while(i > 0 && 0x0A != self->source.buffer[i])
        {
            i--;
        }
        self->position.offset = index(self) - i;
        self->position.line--;
    }
    else
    {
        self->position.offset--;
    }
}
