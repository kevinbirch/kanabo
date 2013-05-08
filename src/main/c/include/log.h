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

#pragma once

#ifdef USE_LOGGING

#include <stdio.h>

enum log_level
{
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    TRACE
};

void enable_logging(void);
void disable_logging(void);
void set_log_level(enum log_level level);
void set_log_level_from_env(void);

#define log_error(COMPONENT, FORMAT, ...)  logger(ERROR, COMPONENT, FORMAT, ##__VA_ARGS__);
#define log_warn(COMPONENT, FORMAT, ...)   logger(WARNING, COMPONENT, FORMAT, ##__VA_ARGS__);
#define log_info(COMPONENT, FORMAT, ...)   logger(INFO, COMPONENT, FORMAT, ##__VA_ARGS__);
#define log_debug(COMPONENT, FORMAT, ...)  logger(DEBUG, COMPONENT, FORMAT, ##__VA_ARGS__);
#define log_trace(COMPONENT, FORMAT, ...)  logger(TRACE, COMPONENT, FORMAT, ##__VA_ARGS__);

#define log_string(LEVEL, COMP, FORMAT, VALUE, LENGTH, ...)             \
    do {                                                                \
    const uint8_t * restrict _log_value = (VALUE);                      \
    const size_t _log_length = (LENGTH);                                \
    char _log_string[_log_length + 1];                                  \
    memcpy(&_log_string, _log_value, _log_length);                      \
    _log_string[_log_length] = '\0';                                    \
    logger(LEVEL, COMP, FORMAT, _log_string, ##__VA_ARGS__);            \
    } while(0)

void logger(enum log_level level, const char * restrict component, const char * restrict format, ...);

#else

#define enable_logging()
#define disable_logging()
#define set_log_level(...)
#define set_log_level_from_env()

#define log_error(...)
#define log_warn(...)
#define log_info(...)
#define log_debug(...)
#define log_trace(...)

#define logger(...)

#endif /* USE_LOGGING */


