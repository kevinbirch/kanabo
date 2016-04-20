
#pragma once


#include "parser/input.h"


struct location_s
{
    String *name;
    size_t  line;
    size_t  offset;
};

typedef struct location_s Location;

#define input_line(INPUT) input_position((INPUT)).line
#define input_offset(INPUT) input_position((INPUT)).offset

#define location_from_input(INPUT) (Location){                          \
        input_name((INPUT)), input_line((INPUT)), input_offset((INPUT)) \
    }
