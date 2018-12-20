#pragma once

#include "maybe.h"

#include "jsonpath.h"
#include "position.h"

enum parser_error_e
{
    INTERNAL_ERROR,
    EMPTY_INPUT,
    PREMATURE_END_OF_INPUT,
    UNCLOSED_QUOTATION,
    UNBALANCED_PRED_DELIM,
    UNSUPPORTED_CONTROL_CHARACTER,
    UNSUPPORTED_ESCAPE_SEQUENCE,
    UNSUPPORTED_UNICODE_SEQUENCE,
    UNEXPECTED_INPUT,
    EXPECTED_QUALIFIED_STEP_PRODUCTION,
    EXPECTED_STEP_PRODUCTION,
    EXPECTED_PREDICATE_PRODUCTION,
    EXPECTED_INTEGER,
    INTEGER_TOO_BIG,
    INTEGER_TOO_SMALL,
    STEP_CANNOT_BE_ZERO,
};

typedef enum parser_error_e ParserErrorCode;

struct parser_error_s
{
    ParserErrorCode code;
    Position        position;
};

typedef struct parser_error_s ParserError;

struct internal_error_s
{
    union
    {
        ParserError base;
        struct parser_error_s;
    };
    const char *filename;
    int         line;
    char        message[];
};

typedef struct internal_error_s ParserInternalError;

make_maybep_error(JsonPath, Vector *);

Maybe(JsonPath) parse(const char *expression);

const char *parser_strerror(ParserErrorCode code);
