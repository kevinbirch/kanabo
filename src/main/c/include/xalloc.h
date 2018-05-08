#pragma once

#include <stdlib.h>

typedef void (*OOMErrorHandler)(size_t, const char * restrict file, int line);

void set_oom_handler(OOMErrorHandler handler);

void *xcalloc_at(size_t size, const char * restrict file, int line);
void  panic_at(const char * restrict message, const char * restrict file, int line) __attribute__((noreturn));

#define xcalloc(SIZE) xcalloc_at(SIZE, __FILE__, __LINE__)
#define panic(MESSAGE) panic_at(MESSAGE, __FILE__, __LINE__)
