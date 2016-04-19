
#include "str.h"

#include "parser/source.h"


void buffer_source_init(BufferSource *self, const char *name, size_t size)
{
    if(NULL != name)
    {
        self->name = make_string(name);
    }
    self->length = size;
}
