#include <string.h>

#include "parser/input_base.h"


Input *make_input_from_buffer(const char *data, size_t length)
{
    Input *self = input_alloc(length, NULL);
    if(NULL == self)
    {
        goto exit;
    }
    input_init(self);

    memcpy(self->source.buffer, data, length);

  exit:
    return self;
}
