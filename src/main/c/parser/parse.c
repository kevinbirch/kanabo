#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "parser/parse.h"

static const char * const FALLBACK_MSG = "message formatting failed";

void add_internal_error(Parser *self, const char *restrict filename, int line, const char * restrict fmt, ...)
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
        vsnprintf(err->message, length, fmt, format_args);
    }
    va_end(format_args);

    err->code = INTERNAL_ERROR;
    err->position = self->scanner->current.location.position;
    err->filename = filename;
    err->line = line;

    vector_append(self->errors, err);
}

void add_error(Parser *self, Position position, ParserErrorCode code)
{
    ParserError *err = xcalloc(sizeof(ParserError));
    err->code = code;
    err->position = position;

    vector_append(self->errors, err);
}

static void error_handler(Position position, ParserErrorCode code, void *parameter)
{
    Parser *self = (Parser *)parameter;
    add_error(self, position, code);
}

Maybe(JsonPath) parse(const char *expression)
{
    Parser parser = {.errors = make_vector_with_capacity(1)};
    if(NULL == expression || 0 == strlen(expression))
    {
        add_error(&parser, (Position){}, EMPTY_INPUT);
        return fail(JsonPath, parser.errors);
    }

    Scanner *scanner = make_scanner(expression, strlen(expression));
    scanner->handler.callback = error_handler;
    scanner->handler.parameter = &parser;

    parser.scanner = scanner;

    JsonPath path = recognize(&parser);

    dispose_scanner(scanner);
    if(vector_is_empty(parser.errors))
    {
        vector_free(parser.errors);
        return just(JsonPath, path);
    }

    dispose_path(path);
    return fail(JsonPath, parser.errors);
}
