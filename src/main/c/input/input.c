#include <ctype.h>
#include <stdlib.h>
#include <string.h>  // for memcmp, memcpy

#include "conditions.h"

#include "input.h"

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
    PRECOND_NONNULL_ELSE_VOID(self);

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
    PRECOND_NONNULL_ELSE_VOID(self);

    string_free(self->name);
}

void dispose_input(Input *self)
{
    PRECOND_NONNULL_ELSE_VOID(self);

    string_free(self->name);
    free(self);
}

String *input_name(Input *self)
{
    PRECOND_NONNULL_ELSE_NULL(self);

    return self->name;
}

size_t input_length(Input *self)
{
    PRECOND_NONNULL_ELSE_ZERO(self);

    return self->source.length;
}

void input_set_track_lines(Input *self, bool value)
{
    PRECOND_NONNULL_ELSE_VOID(self);

    self->track_lines = value;
}

bool input_is_tracking_lines(Input *self)
{
    PRECOND_NONNULL_ELSE_FALSE(self);
    return self->track_lines;
}

Position input_position(const Input *self)
{
    if(NULL == self)
    {
        return (Position){};
    }

    return self->position;
}

SourcePosition input_source_position(const Input *self)
{
    if(NULL == self)
    {
        return (SourcePosition){.name=NULL};
    }

    return (SourcePosition){self->name, self->position};
}

void input_goto(Input *self, Position position)
{
    PRECOND_NONNULL_ELSE_VOID(self);
    PRECOND_ELSE_VOID(position.index < self->source.length);

    self->position = position;
}

void input_advance_to_end(Input *self)
{
    PRECOND_NONNULL_ELSE_VOID(self);

    advance_by(self, input_remaining(self));
}

void input_reset(Input *self)
{
    PRECOND_NONNULL_ELSE_VOID(self);

    self->position.index = 0;
    self->position.line = 0;
    self->position.offset = 0;
}

bool input_has_more(Input *self)
{
    PRECOND_NONNULL_ELSE_FALSE(self);

    return index(self) < self->source.length;
}

size_t input_remaining(Input *self)
{
    PRECOND_NONNULL_ELSE_ZERO(self);

    if(index(self) >= self->source.length)
    {
        return 0;
    }

    return self->source.length - index(self);
}

char input_peek(Input *self)
{
    PRECOND_NONNULL_ELSE_ZERO(self);
    PRECOND_ELSE_ZERO(input_has_more(self));

    return current(self);
}

void input_skip_whitespace(Input *self)
{
    PRECOND_NONNULL_ELSE_VOID(self);
    while(input_has_more(self) && isspace(current(self)))
    {
        incr(self);
    }
}

char input_consume_one(Input *self)
{
    PRECOND_NONNULL_ELSE_ZERO(self);
    PRECOND_ELSE_ZERO(input_has_more(self));

    char current = current(self);
    incr(self);

    return current;
}

size_t input_consume_many(Input *self, size_t count, char *result)
{
    PRECOND_NONNULL_ELSE_ZERO(self);
    PRECOND_ELSE_ZERO(input_has_more(self));

    size_t length = count;
    if(count > input_remaining(self))
    {
        length = input_remaining(self);
    }

    if(result != NULL)
    {
        memcpy(result, cursor(self), length);
    }
    advance_by(self, length);

    return length;
}

bool input_consume_if(Input *self, const char *value)
{
    PRECOND_NONNULL_ELSE_FALSE(self);
    PRECOND_ELSE_FALSE(input_has_more(self));

    size_t length = strlen(value);
    PRECOND_ELSE_FALSE(length <= input_remaining(self));

    if(memcmp(cursor(self), value, length) == 0)
    {
        advance_by(self, length);
        return true;
    }

    return false;
}

void input_push_back(Input *self)
{
    PRECOND_NONNULL_ELSE_VOID(self);
    PRECOND_ELSE_VOID(0 != index(self));

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

char *input_extract(Input *self, Location location)
{
    PRECOND_NONNULL_ELSE_NULL(self);
    PRECOND_ELSE_NULL(location.index < self->source.length);
    PRECOND_ELSE_NULL(location.index + location.extent <= self->source.length);

    char *value = xcalloc(location.extent + 1);
    memcpy(value, self->source.buffer + location.index, location.extent);
    value[location.extent] = '\0';

    return value;
}
