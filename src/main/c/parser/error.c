#include <stdarg.h>

#include "xalloc.h"

#include "parser/recognizer.h"

static const char * const FALLBACK_MSG = "message formatting failed";

void parser_add_error_at(Parser *self, ParserErrorCode code, SourceLocation location, size_t index)
{
    ParserError *err = xcalloc(sizeof(ParserError));
    err->code = code;
    err->location = location;
    err->index = index;

    vector_append(self->errors, err);
}

void parser_add_internal_error_at(Parser *self, const char * restrict location, const char * restrict format, ...)
{
    ParserInternalError *err = xcalloc(sizeof(ParserInternalError));
    err->code = INTERNAL_ERROR;
    err->location = location;

    va_list args;
    va_start(args, format);
    err->message = vformat(format, args);
    va_end(args);

    if(NULL == err->message)
    {
        err->message = make_string(FALLBACK_MSG);
    }

    vector_append(self->errors, err);
}
