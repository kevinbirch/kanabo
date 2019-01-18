#ifdef __linux__
#define _POSIX_C_SOURCE 200112L  // for fileno
#endif

#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>  // for exit

#include "panic.h"

static const char * const PANIC_MESSAGE = "panic - ";

static void print_prelude(const char * restrict file, int line)
{
    if(NULL != file)
    {
        fputs(file, stderr);
        fputs(":", stderr);
        char buf[21];  // len(decimal 64-bit int) + 1
        int len = snprintf(buf, 21, "%d", line);
        fwrite(buf, (size_t)len, 1, stderr);
        fputs(": ", stderr);
    }
    fputs(PANIC_MESSAGE, stderr);
}

void _panic_at(const char * restrict file, int line, const char * restrict message)
{
    print_prelude(file, line);
    fputs(message, stderr);
    fputs("\n", stderr);

    void *stack[20];
    int depth;

    depth = backtrace(stack, 20);
    fputs("Backtrace follows (most recent first):\n", stderr);
    backtrace_symbols_fd(stack, depth, fileno(stderr));

    exit(EXIT_FAILURE);
}

void _panicf_at(const char * restrict file, int line, const char * restrict format, ...)
{
    print_prelude(file, line);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    void *stack[20];
    int depth = backtrace(stack, 20);
    fputs("Backtrace follows (most recent first):\n", stderr);
    backtrace_symbols_fd(stack, depth, fileno(stderr));

    exit(EXIT_FAILURE);
}
