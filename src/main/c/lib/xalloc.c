#include <stdio.h>
#include <stdlib.h>  // for calloc

#include "panic.h"
#include "xalloc.h"

static const char * const NOMEM_MESSAGE = "out of memory! attempted allocation: ";

static OOMErrorHandler custom_handler = NULL;

void set_oom_handler(OOMErrorHandler handler)
{
    custom_handler = handler;
}

void *_xcalloc_at(const char * restrict file, int line, size_t size)
{
    void *obj = calloc(1, size);
    if(NULL != obj)
    {
        return obj;
    }

    char buf[58];   // len(NOMEM_MESSAGE) + len(decimal 64-bit int) + 1
    snprintf(buf, 58, "%s%zu", NOMEM_MESSAGE, size);
    if(NULL != custom_handler)
    {
        custom_handler(file, line, size);
        return NULL;
    }

    _panic_at(file, line, buf);
}
