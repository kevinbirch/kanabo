#include <stdio.h>

#include "emitter/scalar.h"

bool emit_raw_string(const String *value)
{
    return 1 == fwrite(strdta(value), strlen(value), 1, stdout);
}
