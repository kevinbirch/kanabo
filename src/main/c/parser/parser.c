#include <string.h>

#include "xalloc.h"

#include "parser.h"
#include "parser/recognizer.h"

Maybe(JsonPath) parse(const char *expression)
{
    Parser parser = {
        .errors = make_vector_with_capacity(1),
    };

    if(NULL == expression || 0 == strlen(expression))
    {
        parser_add_error(&parser, EMPTY_INPUT);
        return fail(JsonPath, parser.errors);
    }

    parser.input =  make_input_from_buffer(expression, strlen(expression));

    JsonPath *path = recognize(&parser);

    dispose_input(parser.input);

    if(vector_is_empty(parser.errors))
    {
        dispose_vector(parser.errors);
        return just(JsonPath, path);
    }

    dispose_path(path);
    return fail(JsonPath, parser.errors);
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

void parser_release(Parser *self)
{
    vector_destroy(self->errors, freedom_iterator);
    dispose_input(self->input);
}
