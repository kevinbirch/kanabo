#pragma once

typedef void (*OOMErrorHandler)(const char * restrict, size_t);

void set_oom_handler(OOMErrorHandler handler);

void *(xcalloc)(const char * restrict location, size_t size);
void *(xrealloc)(const char * restrict location, void *ptr, size_t new_size);

#define VAL(x) #x
#define STR(x) VAL(x)

#define xcalloc(SIZE) (xcalloc)(__FILE__":"STR(__LINE__), SIZE)
#define xrealloc(PTR, SIZE) (xrealloc)(__FILE__":"STR(__LINE__), PTR, SIZE)
