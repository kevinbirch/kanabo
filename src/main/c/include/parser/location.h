
#pragma once


#include "parser/input.h"


struct location_s
{
    String *name;
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


#define location_from_input(INPUT) (Location){      \
    input_name((INPUT)),                            \
    input_line((INPUT)),                            \
    input_offset((INPUT)),                          \
    0                                               \
}
    
