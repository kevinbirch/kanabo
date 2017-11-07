#include <stdio.h>
#include <string.h>

static OOMErrorHandler custom_handler = NULL;

static const char * const PANIC_MESSAGE = "panic - out of memory!";
static const char * const ALLOCATING_MESSAGE = " allocating ";
static const char * const BYTES_MESSAGE = " bytes\n";

#define xs(V) s(V)
#define s(V) #V
static const size_t SIZE_STR_MAX = sizeof(xs(SIZE_MAX));

static inline void default_oom_handler(size_t size, const char * restrict file, const char * restrict line) __attribute__((noreturn));

static inline void default_oom_handler(size_t size, const char * restrict file, const char * restrict line)
{
    if(NULL != file && NULL != line)
    {
        fwrite(file, strlen(file), 1, stderr);
        fwrite(":", 1, 1, stderr);
        fwrite(line, strlen(line), 1, stderr);
        fwrite(": ", 2, 1, stderr);
    }
    fwrite(PANIC_MESSAGE, strlen(PANIC_MESSAGE), 1, stderr);
    char buf[SIZE_STR_MAX + 1];
    int len = snprintf(buf, SIZE_STR_MAX + 1, "%zu", size);
    if(len > 0)
    {
        fwrite(ALLOCATING_MESSAGE, strlen(ALLOCATING_MESSAGE), 1, stderr);
        fwrite(buf, (size_t)len, 1, stderr);
        fwrite(BYTES_MESSAGE, strlen(BYTES_MESSAGE), 1, stderr);
    }
    else
    {
        fwrite("\n", 1, 1, stderr);
    }

    exit(EXIT_FAILURE);
}

static inline void handle_oom_error(size_t size, const char * restrict file, const char * restrict line)
{
    if(NULL != custom_handler)
    {
        custom_handler(size, file, line);
    }
    else
    {
        default_oom_handler(size, file, line);
    }
}

void set_oom_handler(OOMErrorHandler handler)
{
    custom_handler = handler;
}

void *xcalloc_at(size_t size, const char * restrict file, int line)
{
    void *obj = calloc(1, size);
    if(NULL == obj)
    {
        handle_oom_error(size, file, xs(line));
    }

    return obj;
}
