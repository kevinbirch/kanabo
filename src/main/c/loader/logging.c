#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#ifdef __APPLE__
#define _DARWIN_SOURCE
#endif

#include <stdio.h>
#include <string.h>

#include "loader.h"
#include "loader/private.h"
#include "conditions.h"

static const char * const MESSAGES[] =
{
    "Success",
    "Input was NULL",
    "Input was zero length",
    "No documents found",
    "Unable to allocate memory",
    "An error occured reading the input: %s at %zu",
    "An error occured scanning the input: %s at line %ld, column %ld",
    "An error occured parsing the input: %s at line %ld, column %ld",
    "A non-scalar mapping key was found on line %ld",
    "No matching anchor was found for the alias on line %ld",
    "The alias on line %ld refers to an anchor that is an ancestor",
    "A duplicate mapping key was found on line %ld",
    "An unexpected error has occured."
};

loader_status_code interpret_yaml_error(yaml_parser_t *parser)
{
    switch (parser->error)
    {
        case YAML_NO_ERROR:
            return LOADER_SUCCESS;
        case YAML_MEMORY_ERROR:
            return ERR_LOADER_OUT_OF_MEMORY;
        case YAML_READER_ERROR:
            return ERR_READER_FAILED;
        case YAML_SCANNER_ERROR:
            return ERR_SCANNER_FAILED;
        case YAML_PARSER_ERROR:
            return ERR_PARSER_FAILED;
        default:
            return ERR_OTHER;
    }
}

char *loader_simple_status_message(loader_status_code code)
{
    char *message = NULL;
    switch (code)
    {
        case LOADER_SUCCESS:
        case ERR_INPUT_IS_NULL:
        case ERR_INPUT_SIZE_IS_ZERO:
        case ERR_NO_DOCUMENTS_FOUND:
        case ERR_LOADER_OUT_OF_MEMORY:
        case ERR_OTHER:
            message = strdup(MESSAGES[code]);
            break;
        default:
            message = strdup(MESSAGES[ERR_OTHER]);
    }
    return message;
}

char *loader_status_message(const struct loader_context *context)
{
    PRECOND_NONNULL_ELSE_NULL(context);

    char *message = NULL;
    int result = 0;
    switch (context->code)
    {
        case ERR_READER_FAILED:
            result = asprintf(&message, MESSAGES[context->code], context->parser.problem, context->parser.problem_offset);
            break;
        case ERR_PARSER_FAILED:
        case ERR_SCANNER_FAILED:
            result = asprintf(&message, MESSAGES[context->code], context->parser.problem, context->parser.problem_mark.line+1, context->parser.problem_mark.column+1);
            break;
        case ERR_ALIAS_LOOP:
        case ERR_NO_ANCHOR_FOR_ALIAS:
        case ERR_NON_SCALAR_KEY:
        case ERR_DUPLICATE_KEY:
            result = asprintf(&message, MESSAGES[context->code], context->parser.mark.line);
            break;
        default:
            message = strdup(MESSAGES[context->code]);
            break;
    }
    if(-1 == result)
    {
        message = NULL;
    }

    return message;
}
