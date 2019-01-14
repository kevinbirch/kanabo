#pragma once

typedef void (*OOMErrorHandler)(size_t, const char * restrict file, int line);

void set_oom_handler(OOMErrorHandler handler);

void *_xcalloc_at(size_t size, const char * restrict file, int line);

#define xcalloc(SIZE) _xcalloc_at(SIZE, __FILE__, __LINE__)
