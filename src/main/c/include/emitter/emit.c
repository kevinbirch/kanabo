#include "emitter/emit.h"

bool emit_string(const String *value)
{
    errno = 0;
    if(1 != fwrite(strdta(value), strlen(value), 1, stdout))
    {
        return false;
    }

    return true;
}
