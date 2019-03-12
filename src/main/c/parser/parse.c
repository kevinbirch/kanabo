#include <string.h>

#include "parser/context.h"
#include "parser/recognize.h"
#include "xalloc.h"

static void error_handler(Position position, ParserErrorCode code, void *parameter)
{
    Parser *self = (Parser *)parameter;
    add_parser_error(self, position, code);
}

Maybe(JsonPath) parse(const char *expression)
{
    Parser parser = {.errors = make_vector_with_capacity(1)};

    if(NULL == expression || 0 == strlen(expression))
    {
        add_parser_error(&parser, (Position){}, EMPTY_INPUT);
        return fail(JsonPath, parser.errors);
    }

    Scanner *scanner = make_scanner(expression, strlen(expression));
    scanner->handler.callback = error_handler;
    scanner->handler.parameter = &parser;

    parser.scanner = scanner;

    JsonPath *path = recognize(&parser);

    dispose_scanner(scanner);
    if(vector_is_empty(parser.errors))
    {
        dispose_vector(parser.errors);
        return just(JsonPath, path);
    }

    dispose_path(path);
    return fail(JsonPath, parser.errors);
}
