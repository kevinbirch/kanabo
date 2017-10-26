#include <ctype.h>
#include <stdlib.h>
#include <string.h>  // for memcmp, memcpy

#include "parser/input.h"
#include "parser/input_base.h"


#define current(INPUT) (INPUT)->source.buffer[(INPUT)->position.index]
#define cursor(INPUT) (INPUT)->source.buffer + (INPUT)->position.index
#define index(INPUT) (INPUT)->position.index

Input *input_alloc(size_t bufsize, const char *name)
{
    Input *self = calloc(1, sizeof(Input) + bufsize);
    if(NULL == self)
    {
        return NULL;
    }
    self->marks = make_vector();
    if(NULL == self->marks)
    {
        dispose_input(self);
        self = NULL;
    }
    if(NULL != name)
    {
        self->name = S(name);
    }
    self->source.length = bufsize;

    return self;
}

void input_init(Input *self)
{
    self->position.index = 0;
    self->position.line = 0;
    self->position.offset = 0;
}

static inline bool is_line_break(Input *self)
{
    return 0x0A == current(self);
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
    while(index(self) < self->source.length && count++ < amount)
    {
        if(is_line_break(self))
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
    string_free(self->name);
    vector_destroy(self->marks, free);
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

void input_reset_to_mark(Input *self)
{
    if(vector_is_empty(self->marks))
    {
        return;
    }
    Position *mark = vector_peek(self->marks);
    self->position = *mark;
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

void input_drop_mark(Input *self)
{
    if(vector_is_empty(self->marks))
    {
        return;
    }
    Position *mark = vector_pop(self->marks);
    free(mark);
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
        if(is_line_break(self))
        {
            incr_line(self);
        }
        else
        {
            incr(self);
        }
    }
}

char input_consume_one(Input *self)
{
    if(!input_has_more(self))
    {
        return 0;
    }

    char current = current(self);
    if(is_line_break(self))
    {
        incr_line(self);
    }
    else
    {
        incr(self);
    }

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

    if(1 == self->position.offset)
    {
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
    self->position.index--;
}
