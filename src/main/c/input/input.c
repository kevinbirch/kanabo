#include <ctype.h>
#include <stdlib.h>
#include <string.h>  // for memcmp, memcpy

#include "conditions.h"
#include "input.h"
#include "xalloc.h"

#define read_at(INPUT, IDX) (INPUT)->buffer[(IDX)]
#define read(INPUT) read_at(INPUT, (INPUT)->position.index)

#define cursor_at(INPUT, IDX) (INPUT)->buffer + (IDX)
#define cursor(INPUT) cursor_at(INPUT, (INPUT)->position.index)

#define index(INPUT) (INPUT)->position.index

static inline void incr(Input *self)
{
    if(self->track_lines)
    {
        if(0x0A == read(self))
        {
            self->position.line++;
            self->position.offset = 0;
        }
        else
        {
            self->position.offset++;
        }
    }

    self->position.index++;
}

static inline void advance_by(Input *self, size_t amount)
{
    size_t count = 0;
    while(index(self) < self->length && count++ < amount)
    {
        incr(self);
    }
}

void input_init(Input *self, const char *name, size_t length)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    self->position.index = 0;
    self->position.line = 0;
    self->position.offset = 0;
    self->track_lines = true;

    if(NULL != name)
    {
        self->name = S(name);
    }
    self->length = length;
}

void input_release(Input *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    dispose_string(self->name);
}

void dispose_input(Input *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    dispose_string(self->name);
    free(self);
}

void input_goto(Input *self, Position position)
{
    ENSURE_NONNULL_ELSE_VOID(self);
    ENSURE_ELSE_VOID(position.index < self->length);

    self->position = position;
}

void input_reset(Input *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    self->position.index = 0;
    self->position.line = 0;
    self->position.offset = 0;
}

size_t input_remaining(Input *self)
{
    ENSURE_NONNULL_ELSE_ZERO(self);

    if(index(self) >= self->length)
    {
        return 0;
    }

    return self->length - index(self);
}

void input_skip_whitespace(Input *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    while(input_has_more(self) && isspace(read(self)))
    {
        incr(self);
    }
}

char input_peek(Input *self)
{
    ENSURE_NONNULL_ELSE_ZERO(self);
    ENSURE_ELSE_ZERO(input_has_more(self));

    return read(self);
}

char input_consume_one(Input *self)
{
    ENSURE_NONNULL_ELSE_ZERO(self);
    ENSURE_ELSE_ZERO(input_has_more(self));

    char current = read(self);
    incr(self);

    return current;
}

size_t input_consume_many(Input *self, size_t count, char *result)
{
    ENSURE_NONNULL_ELSE_ZERO(self);
    ENSURE_ELSE_ZERO(input_has_more(self));

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
    ENSURE_NONNULL_ELSE_FALSE(self);
    ENSURE_ELSE_FALSE(input_has_more(self));

    size_t length = strlen(value);
    ENSURE_ELSE_FALSE(length <= input_remaining(self));

    if(memcmp(cursor(self), value, length) == 0)
    {
        advance_by(self, length);
        return true;
    }

    return false;
}

void input_push_back(Input *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);
    ENSURE_ELSE_VOID(0 != index(self));

    self->position.index--;
    if(!self->track_lines)
    {
        return;
    }

    if(0 == self->position.offset)
    {
        // N.B. - track back from the position preceeding the previous newline
        size_t i = index(self) - 1;
        while(i > 0 && 0x0A != read_at(self, i))
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

String *input_extract(Input *self, Location location)
{
    ENSURE_NONNULL_ELSE_NULL(self);
    ENSURE_ELSE_NULL(location.start.index < self->length);
    ENSURE_ELSE_NULL(location.end.index <= self->length);

    return make_string_with_bytestring((const uint8_t *)cursor_at(self, location.start.index), location.end.index);
}
