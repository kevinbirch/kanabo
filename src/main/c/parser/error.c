#include <stdarg.h>
#include <stdio.h>

#include "parser/context.h"
#include "xalloc.h"

static const char * const FALLBACK_MSG = "message formatting failed";

void add_parser_internal_error(Parser *self, const char *restrict filename, int line, const char * restrict fmt, ...)
{
    va_list count_args;
    va_start(count_args, fmt);
    va_list format_args;
    va_copy(format_args, count_args);
    int length = vsnprintf(NULL, 0, fmt, count_args) + 1;
    va_end(count_args);

    ParserInternalError *err;
    if(length < 0)
    {
        size_t fallback_length = strlen(FALLBACK_MSG);
        err = xcalloc(sizeof(ParserInternalError) + fallback_length);
        memcpy(err->message, FALLBACK_MSG, fallback_length);
    }
    else
    {
        err  = xcalloc(sizeof(ParserInternalError) + (size_t)length);
        vsnprintf(err->message, (size_t)length, fmt, format_args);
    }
    va_end(format_args);

    err->code = INTERNAL_ERROR;
    err->position = self->scanner->current.location.position;
    err->filename = filename;
    err->line = line;

    vector_append(self->errors, err);
}


void add_parser_error(Parser *self, Position position, ParserErrorCode code)
{
    ParserError *err = xcalloc(sizeof(ParserError));
    err->code = code;
    err->position = position;

    vector_append(self->errors, err);
}
