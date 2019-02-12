#ifdef __linux__
#define _POSIX_C_SOURCE 200112L  // for fileno
#endif

#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>  // for exit

#include "panic.h"

#define BACKTRACE_MAX_DEPTH 20

static const char * const PANIC_MESSAGE = "panic - ";

static void print_prelude(const char * restrict location)
{
    if(NULL != location)
    {
        fputs(location, stderr);
        fputs(": ", stderr);
    }
    fputs(PANIC_MESSAGE, stderr);
}

noreturn void (panic)(const char * restrict location, const char * restrict message)
{
    print_prelude(location);
    fputs(message, stderr);
    fputs("\n", stderr);

    void *stack[BACKTRACE_MAX_DEPTH];
    int depth = backtrace(stack, BACKTRACE_MAX_DEPTH);
    fputs("Backtrace follows (most recent first):\n", stderr);
    backtrace_symbols_fd(stack, depth, fileno(stderr));

    exit(EXIT_FAILURE);
}

noreturn void (panicf)(const char * restrict location, const char * restrict format, ...)
{
    print_prelude(location);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    void *stack[BACKTRACE_MAX_DEPTH];
    int depth = backtrace(stack, BACKTRACE_MAX_DEPTH);
    fputs("Backtrace follows (most recent first):\n", stderr);
    backtrace_symbols_fd(stack, depth, fileno(stderr));

    exit(EXIT_FAILURE);
}
