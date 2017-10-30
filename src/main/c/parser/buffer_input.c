#include <string.h>

#include "parser/input.h"


Input *make_input_from_buffer(const char *data, size_t length)
{
    Input *self = calloc(1, sizeof(Input) + length);
    if(NULL == self)
    {
        goto exit;
    }
    if(!input_init(self, NULL, length))
    {
        dispose_input(self);
        self = NULL;
        goto exit;
    }

    memcpy(self->source.buffer, data, length);

  exit:
    return self;
}
