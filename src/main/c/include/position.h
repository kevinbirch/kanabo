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
    String   *name;
    Position  position;
};

typedef struct source_position_s SourcePosition;
