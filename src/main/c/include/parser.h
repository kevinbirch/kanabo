#pragma once

#include "maybe.h"

#include "jsonpath.h"
#include "parser/position.h"

enum parser_error_e
{
    PREMATURE_END_OF_INPUT,
    UNSUPPORTED_CONTROL_CHARACTER,
    UNSUPPORTED_ESCAPE_SEQUENCE,
    EMPTY_INPUT,
    EXPECTED_QUALIFIED_STEP_PRODUCTION,
    EXPECTED_PREDICATE_EXPRESSION_PRODUCTION,
    EXPECTED_STEP_PRODUCTION,
    ERR_UNBALANCED_PRED_DELIM,       // missing closing predicate delimiter `]'
    ERR_EXPECTED_INTEGER,            // expected an integer
    ERR_INVALID_NUMBER,              // invalid number
    ERR_STEP_CANNOT_BE_ZERO,         // slice step value must be non-zero
};

typedef enum parser_error_e ParserErrorCode;

struct parser_error_s
{
    ParserErrorCode code;
    Position position;
};

typedef struct parser_error_s ParserError;

make_maybe_error(JsonPath, Vector *);

Maybe(JsonPath) parse(const char *expression);

const char *parser_strerror(ParserErrorCode code);
