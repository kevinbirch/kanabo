#pragma once

#include "position.h"

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

struct source_location_s
{
    String   *name;
    Location  location;
};

typedef struct source_location_s SourceLocation;
