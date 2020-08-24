#pragma once

#include <stddef.h>

#include "str.h"

struct postion_s
{
    size_t index;
    size_t line;
    size_t offset;
};

typedef struct postion_s Position;

struct source_position_s
{
    union
    {
        struct    postion_s;
        Position  position;
    };
    String   *name;
};

typedef struct source_position_s SourcePosition;
