#pragma once

#include <stddef.h>

struct postion_s
{
    size_t index;
    size_t line;
    size_t offset;
};

typedef struct postion_s Position;
