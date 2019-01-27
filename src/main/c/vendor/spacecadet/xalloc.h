#pragma once

typedef void (*OOMErrorHandler)(const char * restrict, size_t);

void set_oom_handler(OOMErrorHandler handler);

void *_xcalloc_at(const char * restrict location, size_t size);

#define VAL(x) #x
#define STR(x) VAL(x)

#define xcalloc(SIZE) _xcalloc_at(__FILE__":"STR(__LINE__), SIZE)
