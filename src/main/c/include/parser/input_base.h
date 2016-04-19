
#pragma once


#include "vector.h"

#include "parser/source.h"


struct input_s
{
    Position      position;
    Vector       *marks;
    BufferSource  source;
};


Input *input_alloc(size_t bufsiz);
void   input_init(Input *self, const char *name, size_t size);
