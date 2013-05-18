/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 *
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README file in the project root.
 *
 * Distributed under an [MIT-style][license] license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimers in the documentation and/or
 *   other materials provided with the distribution.
 * - Neither the names of the copyright holders, nor the names of the authors, nor
 *   the names of other contributors may be used to endorse or promote products
 *   derived from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/ncsa
 */

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

static bool LOGGING_ENABLED = false;
static enum log_level LOG_LEVEL = ERROR;

int print_prelude(enum log_level level, const char * restrict component);

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
        set_log_level(INFO);
    }
    else if(0 == memcmp("ERROR", level, 5))
    {
        set_log_level(ERROR);
    }
    else if(0 == memcmp("WARNING", level, 7))
    {
        set_log_level(WARNING);
    }
    else if(0 == memcmp("INFO", level, 4))
    {
        set_log_level(INFO);
    }
    else if(0 == memcmp("DEBUG", level, 5))
    {
        set_log_level(DEBUG);
    }
    else if(0 == memcmp("TRACE", level, 5))
    {
        set_log_level(TRACE);
    }
    else
    {
        set_log_level(INFO);
    }
}

#define ensure_log_enabled(LEVEL) if(!LOGGING_ENABLED || LOG_LEVEL < LEVEL) return 0

int logger(enum log_level level, const char * restrict component, const char * restrict format, ...)
{
    ensure_log_enabled(level);
    va_list args;
    va_start(args, format);
    int result = vlogger(level, component, format, args);
    va_end(args);
    return result;
}

int vlogger(enum log_level level, const char * restrict component, const char * restrict format, va_list args)
{
    ensure_log_enabled(level);
    int result = print_prelude(level, component);
    if(!result)
    {
        return result;
    }
    result += vfprintf(stderr, format, args);
    if('\n' != format[strlen(format) - 1])
    {
        result += fprintf(stderr, "\n");
    }
    return result;
}

int print_prelude(enum log_level level, const char * restrict component)
{
    time_t now = time(NULL);
    struct tm now_tm;
    localtime_r(&now, &now_tm);
    return fprintf(stderr, "%d-%d-%d %02d:%02d:%02d %s %s - ", now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday, 
                   now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, LEVELS[level], component);
}

#endif
