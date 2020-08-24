#include "conditions.h"

const void * SENTINEL = (void *)"SENTINEL";

bool cond_is_null(const void *first, ...)
{
    va_list args;
    bool result = false;

    va_start(args, first);
    for(const void *arg = first; arg != SENTINEL; arg = va_arg(args, void *))
    {
        if(NULL == arg)
        {
            result = true;
            break;
        }
    }
    va_end(args);

    return result;
}

bool cond_is_false(int first, ...)
{
    va_list args;
    bool result = false;

    va_start(args, first);
    for(int arg = first; arg != -1; arg = va_arg(args, int))
    {
        if(0 == arg)
        {
            result = true;
            break;
        }
    }
    va_end(args);

    return result;
}
