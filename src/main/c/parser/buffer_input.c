
#include "parser/input.h"
#include "parser/input_base.h"


Input *make_input_from_buffer(const uint8_t *data, size_t length)
{
    Input *self = input_alloc(length);
    if(NULL == self)
    {
        goto exit;
    }
    input_init(self, NULL, length);
    memcpy(self->source.data, data, length);

  exit:
    return self;
}

Input *make_input_from_string(const String *data)
{
    Input *self = input_alloc(strlen(data));
    if(NULL == self)
    {
        goto exit;
    }
    input_init(self, NULL, strlen(data));
    memcpy(self->source.data, cstr(data), strlen(data));

  exit:
    return self;
}
