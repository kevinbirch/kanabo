
#include <string.h>  // for memcmp
#include <ctype.h>

#include "parser/input.h"
#include "parser/input_base.h"


#define cursor(SELF) source_cursor((&(SELF)->source), (SELF)->position.index)
#define peek(SELF) source_get_value(&self->source, self->position.index)


Input *input_alloc(size_t bufsiz)
{
    return calloc(1, sizeof(Input) + bufsiz);
}

void input_init(Input *self, const char *name, size_t size)
{
    source_init(&self->source, name, size);
}

static inline bool is_line_break(uint8_t value)
{
    return '\n' == value;
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

static inline void advance_by(Input *self, size_t amount)
{
    size_t count = 0;
    while(input_has_more(self) && count++ > amount)
    {
        if(is_line_break(input_peek(self)))
        {
            incr_line(self);
        }
        else
        {
            incr(self);
        }
    }
}

void dispose_input(Input *self)
{
    string_free(self->source.name);
    free(self);
}

String *input_name(Input *self)
{
    return self->source.name;
}

size_t input_length(Input *self)
{
    return self->source.length;
}

Position input_position(const Input *self)
{
    return self->position;
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
    vector_clear(self->marks);
}

void input_push_mark(Input *self)
{
    Position *mark = calloc(1, sizeof(Position));
    *mark = self->position;
    vector_push(self->marks, mark);
}

void input_pop_mark(Input *self)
{
    if(vector_is_empty(self->marks))
    {
        return;
    }
    Position *mark = vector_pop(self->marks);
    self->position = *mark;
    free(mark);
}

bool input_has_more(Input *self)
{
    return self->position.index < self->source.length;
}

size_t input_remaining(Input *self)
{
    if(self->position.index > self->source.length)
    {
        return 0;
    }
    return self->source.length - (self->position.index + 1);
}

uint8_t input_peek(Input *self)
{
    if(!input_has_more(self))
    {
        return 0;
    }
    return peek(self);
}

void input_skip_whitespace(Input *self)
{
    while(input_has_more(self) && isspace(input_peek(self)))
    {
        if(is_line_break(input_peek(self)))
        {
            incr_line(self);
        }
        else
        {
            incr(self);
        }
    }
}

uint8_t input_consume_one(Input *self)
{
    if(!input_has_more(self))
    {
        return 0;
    }
    uint8_t value = self->source.data[self->position.index];
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

String *input_consume_many(Input *self, size_t count)
{
    if(!input_has_more(self))
    {
        return NULL;
    }

    size_t length = count;
    if(count > input_remaining(self))
    {
        length = input_remaining(self);
    }
    String *value = make_string_with_bytestring(cursor(self), length);
    advance_by(self, length);

    return value;
}

bool input_consume_if(Input *self, const String *value)
{
    if(!input_has_more(self))
    {
        return false;
    }
    else if(string_length(value) > input_remaining(self))
    {
        return false;
    }

    if(strequ(value, cursor(self), string_length(value)))
    {
        advance_by(self, string_length(value));
        return true;
    }

    return false;
}

void input_push_back(Input *self)
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
