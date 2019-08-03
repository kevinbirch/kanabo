#pragma once

#include "position.h"

struct location_s
{
    union
    {
        struct   postion_s;
        Position position;
    };
    size_t  extent;  //!< the length from `.position.index`
};

typedef struct location_s Location;

struct source_location_s
{
    union
    {
        struct   location_s;
        Location location;        
    };
    String   *name;
};

typedef struct source_location_s SourceLocation;
