#include <string.h>

#include "conditions.h"
#include "input.h"
#include "xalloc.h"

Input *make_input_from_buffer(const char *data, size_t length)
{
    PRECOND_NONNULL_ELSE_NULL(data);
    PRECOND_ELSE_NULL(0 != length);

    Input *self = xcalloc(sizeof(Input) + length);
    input_init(self, NULL, length);

    memcpy(self->source.buffer, data, length);

    return self;
}
