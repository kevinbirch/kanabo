#include <string.h>

#include "xalloc.h"

#include "parser.h"
#include "parser/recognizer.h"

Maybe(JsonPath) parse(const char *expression)
{
    Parser parser = {
        .errors = make_vector_with_capacity(1),
        .input = {
            .source = {
                .cache = false,
                .ref = expression,
            },
        },
    };

    if(NULL == expression || 0 == strlen(expression))
    {
        parser_add_error(&parser, EMPTY_INPUT);
        return fail(JsonPath, parser.errors);
    }

    input_init(&parser.input, NULL, strlen(expression));
    input_set_track_lines(&parser.input, true);

    JsonPath *path = recognize(&parser);

    input_release(&parser.input);

    if(vector_is_empty(parser.errors))
    {
        dispose_vector(parser.errors);
        return just(JsonPath, path);
    }

    dispose_path(path);
    return fail(JsonPath, parser.errors);
}
