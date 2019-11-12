#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "str.h"

bool emit_string(const String *value);

#define EMIT(STR)                                                       \
    do {                                                                \
        errno = 0;                                                      \
        if(EOF == fputs((STR), stdout))                                 \
        {                                                               \
            return false;                                               \
        }                                                               \
    } while(0)

