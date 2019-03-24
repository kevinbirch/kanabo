#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "str.h"

bool emit_raw_string(const String *value);

#define EMIT(STR)                                                       \
    do {                                                                \
        errno = 0;                                                      \
        if(EOF == fputs((STR), stdout))                                 \
        {                                                               \
            return false;                                               \
        }                                                               \
    } while(0)

#define emit_string(VALUE)                                              \
    ({                                                                  \
        bool success = true;                                            \
        errno = 0;                                                      \
        if(1 != fwrite(strdta(VALUE), strlen(VALUE), 1, stdout))        \
        {                                                               \
            success = false;                                            \
        }                                                               \
        success;                                                        \
    })
