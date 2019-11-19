#pragma once

#include "position.h"

struct location_s
{
    Position start;
    Position end;
};

typedef struct location_s Location;

struct source_location_s
{
    union
    {
        struct   location_s;
        Location location;        
    };
    String *name;
};

typedef struct source_location_s SourceLocation;
