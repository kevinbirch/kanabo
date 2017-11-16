#include "parser/parse.h"

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
