#include "parser/errors.h"

static const char * const ERRORS[] =
{
    [PREMATURE_END_OF_INPUT] = "premature end of input",
    [UNSUPPORTED_CONTROL_CHARACTER] = "unsupported control character",
    [UNSUPPORTED_ESCAPE_SEQUENCE] = "unsupported escape sequence"
};

const char *parser_strerror(ParserErrorCode code)
{
    return ERRORS[code];
}

