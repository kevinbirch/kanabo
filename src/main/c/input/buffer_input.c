#include <string.h>

#include "conditions.h"
#include "input.h"
#include "xalloc.h"

Input *make_input_from_buffer(const char *data, size_t length)
{
    ENSURE_NONNULL_ELSE_NULL(data);
    ENSURE_ELSE_NULL(0 != length);

    Input *self = xcalloc(sizeof(Input) + length);
    input_init(self, NULL, length);
    self->source.cache = true;

    memcpy(self->source.buffer, data, length);

    return self;
}

Input *make_input_from_buffer_no_copy(const char *data, size_t length)
{
    ENSURE_NONNULL_ELSE_NULL(data);
    ENSURE_ELSE_NULL(0 != length);

    Input *self = xcalloc(sizeof(Input));
    input_init(self, NULL, length);
    self->source.cache = false;

    self->source.ref = data;

    return self;
}
