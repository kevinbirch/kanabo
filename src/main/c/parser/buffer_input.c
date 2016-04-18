
#include "parser/input.h"
#include "parser/input_base.h"


struct buffer_input_s
{
    union
    {
        struct input_s;
        Input base;
    };
    Source source;
};


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

void dispose_buffer_input(BufferInput *self)
{
    free(self);
}

String *buffer_name(BufferInput *self __attribute__((unused)))
{
    return NULL;
}

Postion buffer_position(const BufferInput *self)
{
    return position(super(self));
}

void buffer_advance_to_end(BufferInput *self)
{
    advance_to_end(super(self), source(self));
}

void buffer_rewind(BufferInput *self)
{
    reset(super(self));
}

void buffer_set_mark(BufferInput *self)
{
    set_mark(super(self));
}

void buffer_reset_to_mark(BufferInput *self)
{
    reset_to_mark(super(self));
}

bool buffer_has_more(BufferInput *self)
{
    return has_more(super(self), source(self));
}

size_t buffer_remaining(BufferInput *self)
{
    return remaining(super(self), source(self));
}

uint8_t buffer_peek(BufferInput *self)
{
    return peek(super(self), source(self));
}

void buffer_skip_whitespace(BufferInput *self)
{
    skip_whitespace(super(self), source(self));
}

uint8_t buffer_consume_one(BufferInput *self)
{
    return consume_one(super(self), source(self));
}

String *buffer_consume_many(BufferInput *self, size_t count)
{
    return consume_many(super(self), source(self), count);
}

bool buffer_consume_if(BufferInput *self, const String *value)
{
    return consume_if(super(self), source(self), value);
}

void buffer_push_back(BufferInput *self)
{
    push_back(super(self), source(self));
}
