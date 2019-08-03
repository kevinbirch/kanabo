#include <stdarg.h>

#include "xalloc.h"

#include "parser.h"

static const char * const FALLBACK_MSG = "message formatting failed";

void parser_add_error_at(Parser *self, ParserErrorCode code, Position position)
{
    ParserError *err = xcalloc(sizeof(ParserError));
    err->code = code;
    err->position = position;

    vector_append(self->errors, err);
}

void parser_add_internal_error_at(Parser *self, const char * restrict location, const char * restrict format, ...)
{
    ParserInternalError *err = xcalloc(sizeof(ParserInternalError));
    err->code = INTERNAL_ERROR;
    err->position = position(self);
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

static void freedom_iterator(void *each)
{
    ParserError *err = (ParserError *)each;
    if(INTERNAL_ERROR == err->code)
    {
        ParserInternalError *ierr = (ParserInternalError *)err;
        dispose_string(ierr->message);
    }

    free(each);
}

void parser_dispose_errors(Vector *errors)
{
    vector_destroy(errors, freedom_iterator);
}
