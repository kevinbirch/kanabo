#include "parser.h"
#include "parser/parse.h"
#include "parser/recognizer.h"

void add_error(Parser *self, Position position, ParserErrorCode code)
{
    ParserError *err = xcalloc(sizeof(ParserError));
    err->code = code;
    err->position = position;

    if(NULL == self->errors)
    {
        self->errors = make_vector_of(1, err);
    }
    else
    {
        vector_append(self->errors, err);
    }
}

static void error_handler(Position position, ParserErrorCode code, void *parameter)
{
    Parser *self = (Parser *)parameter;
    add_error(self, position, code);
}

// xxx MaybeJsonPath, MaybeAst point to vector of errors
void parse(const char *expression)
{
    Scanner *scanner = make_scanner(expression, strlen(expression));

    // xxx - move error handling declarations to parser/error.h?
    // do we need a parser struct at all?
    Parser parser = {.scanner = scanner};

    scanner->handler.callback = error_handler;
    scanner->handler.parameter = &parser;

    /* AstNode *root =  */recognize(&parser);

    // xxx print out errors

    dispose_scanner(scanner);
}
