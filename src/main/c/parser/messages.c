#include "parser.h"

static const char * const ERRORS[] =
{
    [INTERNAL_ERROR] = "parser: internal error",
    [EMPTY_INPUT] = "parser: parsing expression: no expression provided",
    [PREMATURE_END_OF_INPUT] = "parser: parsing expression: premature end of input",
    [UNCLOSED_QUOTATION] = "parser: parsing expression: unclosed quote",
    [UNBALANCED_PRED_DELIM] = "parser: parsing expression: unclosed predicate",
    [UNSUPPORTED_CONTROL_CHARACTER] = "parser: parsing expression: unsupported control character",
    [UNSUPPORTED_ESCAPE_SEQUENCE] = "parser: parsing expression: unsupported escape sequence",
    [UNSUPPORTED_UNICODE_SEQUENCE] = "parser: parsing expression: unsupported unicode sequence",
    [UNEXPECTED_INPUT] = "parser: parsing expression: unexpected input",
    [EXPECTED_QUALIFIED_STEP_PRODUCTION] = "parser: parsing expression: expected qualified step definition",
    [EXPECTED_PREDICATE_PRODUCTION] = "parser: parsing expression: expected predicate definition",
    [EXPECTED_STEP_PRODUCTION] = "parser: parsing expression: expected step selector or transformer definition",
    [EXPECTED_INTEGER] = "parser: parsing expression: expected integer literal",
    [INTEGER_TOO_BIG] = "parser: parsing expression: integer literal is too large to be represented",
    [INTEGER_TOO_SMALL] = "parser: parsing expression: integer literal is too small to be represented",
    [STEP_CANNOT_BE_ZERO] = "parser: parsing expression: slice step value cannot be zero",
};

const char *parser_strerror(ParserErrorCode code)
{
    return ERRORS[code];
}

