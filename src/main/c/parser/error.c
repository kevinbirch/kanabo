#include <stdarg.h>
#include <stdio.h>

#include "parser/context.h"
#include "xalloc.h"

static const char * const FALLBACK_MSG = "message formatting failed";

void add_parser_internal_error(Parser *self, const char *restrict filename, int line, const char * restrict format, ...)
{
    ParserInternalError *err = xcalloc(sizeof(ParserInternalError));
    err->code = INTERNAL_ERROR;
    err->position = self->scanner->current.location.position;
    err->filename = filename;
    err->line = line;

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


void add_parser_error(Parser *self, Position position, ParserErrorCode code)
{
    ParserError *err = xcalloc(sizeof(ParserError));
    err->code = code;
    err->position = position;

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
