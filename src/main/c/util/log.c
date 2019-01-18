#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "log.h"

#ifdef USE_LOGGING

static const char * const LEVELS[] =
{
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE"
};

static bool LOGGING_ENABLED = true;
static enum log_level LOG_LEVEL = LVL_ERROR;

int print_prelude(enum log_level level, const char *component);

void enable_logging(void)
{
    LOGGING_ENABLED = true;
}

void disable_logging(void)
{
    LOGGING_ENABLED = false;
}

void set_log_level(enum log_level level)
{
    LOG_LEVEL = level;
}

void set_log_level_from_env(void)
{
    char *level = getenv("KANABO_LOG_LEVEL");
    if(NULL == level)
    {
        set_log_level(LVL_INFO);
    }
    else if(0 == memcmp("ERROR", level, 5))
    {
        set_log_level(LVL_ERROR);
    }
    else if(0 == memcmp("WARNING", level, 7))
    {
        set_log_level(LVL_WARNING);
    }
    else if(0 == memcmp("INFO", level, 4))
    {
        set_log_level(LVL_INFO);
    }
    else if(0 == memcmp("DEBUG", level, 5))
    {
        set_log_level(LVL_DEBUG);
    }
    else if(0 == memcmp("TRACE", level, 5))
    {
        set_log_level(LVL_TRACE);
    }
    else
    {
        set_log_level(LVL_INFO);
    }
}

#define ensure_log_enabled(LEVEL) if(!LOGGING_ENABLED || LOG_LEVEL < LEVEL) return 0

int logger(enum log_level level, const char *component, const char *format, ...)
{
    ensure_log_enabled(level);
    if(NULL == format || 0 == strlen(format))
    {
        return -1;
    }
    va_list args;
    va_start(args, format);
    int result = vlogger(level, component, format, args);
    va_end(args);
    return result;
}

int vlogger(enum log_level level, const char *component, const char *format, va_list args)
{
    ensure_log_enabled(level);
    if(NULL == format || 0 == strlen(format))
    {
        return -1;
    }
    int result = print_prelude(level, component);
    if(!result)
    {
        return result;
    }
    result += vfprintf(stderr, format, args);
    if('\n' != format[strlen(format) - 1])  // strlen will be non-zero here
    {
        result += fprintf(stderr, "\n");
    }
    return result;
}

int print_prelude(enum log_level level, const char *component)
{
    time_t now = time(NULL);
    struct tm *now_tm = localtime(&now);
    return fprintf(stderr, "%d-%d-%d %02d:%02d:%02d %s %s - ",
                   now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday,
                   now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, LEVELS[level], component);
}

#endif
