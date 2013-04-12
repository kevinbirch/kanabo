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

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

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

void enable_logging()
{
    LOGGING_ENABLED = true;
}

void disable_logging()
{
    LOGGING_ENABLED = false;
}

void set_log_level(enum log_level level)
{
    LOG_LEVEL = level;
}

void logger(enum log_level level, const char * restrict component, const char * restrict format, ...)
{
    if(!LOGGING_ENABLED || LOG_LEVEL < level)
    {
        return;
    }
    struct tm now;
    localtime_r(time(NULL), &now);
    int result = fprintf(stderr, "%d-%d-%d %02d:%02d:%02d %s %s", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, 
                         now.tm_hour, now.tm_min, now.tm_sec, LEVELS[level], component);
    if(-1 == result)
    {
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

#endif
