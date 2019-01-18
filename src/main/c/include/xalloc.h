#pragma once

typedef void (*OOMErrorHandler)(const char * restrict, int, size_t);

void set_oom_handler(OOMErrorHandler handler);

void *_xcalloc_at(const char * restrict file, int line, size_t size);

#define xcalloc(SIZE) _xcalloc_at(__FILE__, __LINE__, SIZE)
