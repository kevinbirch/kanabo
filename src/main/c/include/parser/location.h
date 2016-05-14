
#pragma once


#include "parser/input.h"


struct location_s
{
    size_t  line;
    size_t  offset;
    size_t  extent;
};

typedef struct location_s Location;

struct source_location_s
{
    Input   *input;
    Location location;
};

typedef struct source_location_s SourceLocation;


#define location_from_input(INPUT) (SourceLocation){      \
        .input=(INPUT),                                   \
            .location.line=input_line((INPUT)),           \
            .location.offset=input_offset((INPUT)),       \
            .location.extent=0                            \
            }
    
