#include <stdio.h>
#include <stdlib.h>  // for calloc
#include <stdnoreturn.h>
#include <string.h>

#include "panic.h"
#include "xalloc.h"

#define MSG_BUFSZ 64
#define NOMEM_FMT "out of memory! attempted allocation: %zu bytes"

static OOMErrorHandler custom_handler = NULL;

void set_oom_handler(OOMErrorHandler handler)
{
    custom_handler = handler;
}

static inline void handle_oom(const char * restrict location, size_t size)
{
    if(NULL != custom_handler)
    {
        custom_handler(location, size);
        return;
    }

    char buf[MSG_BUFSZ];
    snprintf(buf, MSG_BUFSZ, NOMEM_FMT, size);

    (panic)(location, buf);
}

void *(xcalloc)(const char * restrict location, size_t size)
{
    void *obj = calloc(1, size);
    if(NULL != obj)
    {
        return obj;
    }

    handle_oom(location, size);
    return NULL;
}

void *(xrealloc)(const char * restrict location, void *ptr, size_t new_size)
{
    void *obj = realloc(ptr, new_size);
    if(NULL != obj)
    {
        return obj;
    }

    handle_oom(location, new_size);
    return NULL;
}
