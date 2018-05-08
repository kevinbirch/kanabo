#include <stdio.h>
#include <string.h>

static OOMErrorHandler custom_handler = NULL;

static const char * const PANIC_MESSAGE = "panic - ";
static const char * const NOMEM_MESSAGE = "out of memory! attempted allocation: ";

static inline void signal_panic(const char * restrict message, const char * restrict file, int line) __attribute__((noreturn));

static inline void signal_panic(const char * restrict message, const char * restrict file, int line)
{
    if(NULL != file)
    {
        char buf[21];  // len(decimal 64-bit int) + 1
        int len = snprintf(buf, 21, "%d", line);
        fwrite(file, strlen(file), 1, stderr);
        fwrite(":", 1, 1, stderr);
        fwrite(buf, (size_t)len, 1, stderr);
        fwrite(": ", 2, 1, stderr);
    }
    fwrite(PANIC_MESSAGE, strlen(PANIC_MESSAGE), 1, stderr);
    fwrite(message, strlen(message), 1, stderr);
    fwrite("\n", 1, 1, stderr);

    exit(EXIT_FAILURE);
}

void set_oom_handler(OOMErrorHandler handler)
{
    custom_handler = handler;
}

void *xcalloc_at(size_t size, const char * restrict file, int line)
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
        custom_handler(size, file, line);
        return NULL;
    }

    signal_panic(buf, file, line);
}

void panic_at(const char * restrict message, const char * restrict file, int line)
{
    signal_panic(message, file, line);
}
