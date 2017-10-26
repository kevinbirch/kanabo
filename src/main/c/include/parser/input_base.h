#pragma once


#include <stddef.h>

#include "str.h"
#include "vector.h"

#include "parser/input.h"


struct source_s
{
    size_t length;
    char   buffer[];
};

typedef struct source_s Source;

struct input_s
{
    String   *name;
    Position  position;
    Vector   *marks;
    bool     lines;
    Source   source;
};


Input *input_alloc(size_t bufsize, const char *name);
void input_init(Input *self);
