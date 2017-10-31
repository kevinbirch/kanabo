#pragma once

#include "parser/position.h"

struct location_s
{
    union
    {
        struct postion_s;
        Position position;
    };
    size_t  extent;
};

typedef struct location_s Location;
