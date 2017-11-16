#include "parser.h"

static const char * const ERRORS[] =
{
    [PREMATURE_END_OF_INPUT] = "premature end of input",
    [UNSUPPORTED_CONTROL_CHARACTER] = "unsupported control character",
    [UNSUPPORTED_ESCAPE_SEQUENCE] = "unsupported escape sequence",
    [EMPTY_INPUT] = "no expression provided",
    [EXPECTED_QUALIFIED_STEP_PRODUCTION] = "expected step definition",
    [EXPECTED_PREDICATE_EXPRESSION_PRODUCTION] = "expected predicate definition",
    [EXPECTED_STEP_PRODUCTION] = "expected step selector or transformer definition",
};

const char *parser_strerror(ParserErrorCode code)
{
    return ERRORS[code];
}

