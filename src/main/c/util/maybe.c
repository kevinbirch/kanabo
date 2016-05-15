
#include <stdlib.h>

#include "maybe.h"


void *from_maybe(void *def, Maybe a)
{
    if(is_nothing(a))
    {
        return def;
    }
    return from_just(a);
}


void *maybe(void *def, maybe_fn fn, Maybe a)
{
    if(is_nothing(a))
    {
        return def;
    }
    return fn(from_just(a));
}


Maybe bind(Maybe a, bind_fn fn)
{
    if(is_nothing(a))
    {
        return a;
    }
    void *result = NULL;
    uint_fast16_t code = fn(from_just(a), &result);
    if(NULL == result)
    {
        return fail(code);
    }
    return just(result);
}


Maybe then(Maybe a, then_fn fn)
{
    if(is_nothing(a))
    {
        return a;
    }
    void *result = NULL;
    uint_fast16_t code = fn(&result);
    if(NULL == result)
    {
        return fail(code);
    }
    return just(result);
}


Maybe mplus(Maybe a, Maybe b, mplus_fn fn)
{
    if(is_nothing(a) && is_nothing(b))
    {
        return nothing();
    }
    else if(is_nothing(a) && is_just(b))
    {
        return b;
    }
    else if(is_just(a) && is_nothing(b))
    {
        return a;
    }
    else
    {
        void *result = NULL;
        uint_fast16_t code = fn(from_just(a), from_just(b), &result);
        if(NULL == result)
        {
            return fail(code);
        }
        return just(result);
    }
}
