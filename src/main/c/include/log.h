#pragma once

#define _STRINGIFY(VALUE) #VALUE
#define STRFY(VALUE) _STRINGIFY(VALUE)

#ifdef USE_LOGGING

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enum log_level
{
    LVL_ERROR,
    LVL_WARNING,
    LVL_INFO,
    LVL_DEBUG,
    LVL_TRACE
};

void enable_logging(void);
void disable_logging(void);
void set_log_level(enum log_level level);
void set_log_level_from_env(void);

#define log_error(COMPONENT, FORMAT, ...)  logger(LVL_ERROR, COMPONENT, FORMAT, ##__VA_ARGS__)
#define log_warn(COMPONENT, FORMAT, ...)   logger(LVL_WARNING, COMPONENT, FORMAT, ##__VA_ARGS__)
#define log_info(COMPONENT, FORMAT, ...)   logger(LVL_INFO, COMPONENT, FORMAT, ##__VA_ARGS__)
#define log_debug(COMPONENT, FORMAT, ...)  logger(LVL_DEBUG, COMPONENT, FORMAT, ##__VA_ARGS__)
#define log_trace(COMPONENT, FORMAT, ...)  logger(LVL_TRACE, COMPONENT, FORMAT, ##__VA_ARGS__)

#define log_string(LEVEL, COMP, FORMAT, VALUE, LENGTH, ...) do {        \
        const uint8_t *_log_value = (VALUE);                            \
        const size_t _log_length = (LENGTH);                            \
        char _log_string[_log_length + 1];                              \
        memcpy(&_log_string, _log_value, _log_length);                  \
        _log_string[_log_length] = '\0';                                \
        logger(LEVEL, COMP, FORMAT, _log_string, ##__VA_ARGS__);        \
    } while(0)

int logger(enum log_level level, const char *component, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
int vlogger(enum log_level level, const char *component, const char *format, va_list args) __attribute__ ((format (printf, 3, 0)));

#else

#define LVL_ERROR NULL
#define LVL_WARNING NULL
#define LVL_INFO NULL
#define LVL_DEBUG NULL
#define LVL_TRACE NULL

#define enable_logging()
#define disable_logging()
#define set_log_level(...)
#define set_log_level_from_env()

#define log_error(...)
#define log_warn(...)
#define log_info(...)
#define log_debug(...)
#define log_trace(...)

#define log_string(...)

#define logger(...)
#define vlogger(...)

#endif /* USE_LOGGING */
