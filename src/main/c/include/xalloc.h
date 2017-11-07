#pragma once

#include <stdlib.h>

typedef void (*OOMErrorHandler)(size_t, const char * restrict file, const char * restrict line);

void set_oom_handler(OOMErrorHandler handler);

void *xcalloc_at(size_t size, const char * restrict file, int line);

#define xcalloc(SIZE) xcalloc_at(SIZE, __FILE__, __LINE__)
