#include "parser.h"

static const char * const ERRORS[] =
{
    [INTERNAL_ERROR] = "parser: internal error",
    [EMPTY_INPUT] = "parser: internal error: no expression provided",
    [PREMATURE_END_OF_INPUT] = "premature end of input",
    [UNCLOSED_QUOTATION] = "unclosed quote",
    [UNBALANCED_PRED_DELIM] = "unclosed predicate",
    [UNSUPPORTED_CONTROL_CHARACTER] = "unsupported control character",
    [UNSUPPORTED_ESCAPE_SEQUENCE] = "unsupported escape sequence",
    [UNSUPPORTED_UNICODE_SEQUENCE] = "unsupported unicode sequence",
    [UNEXPECTED_INPUT] = "unexpected input",
    [EXPECTED_QUALIFIED_STEP_PRODUCTION] = "expected qualified step definition",
    [EXPECTED_PREDICATE_PRODUCTION] = "expected predicate definition",
    [EXPECTED_STEP_PRODUCTION] = "expected step selector or transformer definition",
    [EXPECTED_INTEGER] = "expected integer literal",
    [INTEGER_TOO_BIG] = "integer literal is too large to be represented",
    [INTEGER_TOO_SMALL] = "integer literal is too small to be represented",
    [STEP_CANNOT_BE_ZERO] = "slice step value cannot be zero",
};

const char *parser_strerror(ParserErrorCode code)
{
    return ERRORS[code];
}

