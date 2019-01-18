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
            log_error(component, "uh oh! couldn't emit literal \"%s\": %s", (STR), strerror(errno)); \
            return false;                                               \
        }                                                               \
    } while(0)

#define emit_string(VALUE)                                              \
    ({                                                                  \
        bool success = true;                                            \
        errno = 0;                                                      \
        if(1 != fwrite(strdta(VALUE), strlen(VALUE), 1, stdout))        \
        {                                                               \
            log_error(component, "uh oh! couldn't emit scalar \"%s\": %s", C((VALUE)), strerror(errno)); \
            success = false;                                            \
        }                                                               \
        success;                                                        \
    })
