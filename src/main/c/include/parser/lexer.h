#pragma once


#include "parser/input.h"
#include "parser/token.h"


enum lexer_failures_e
{
    UNEXPECTED_INPUT,
    PREMATURE_END_OF_INPUT,
    MISSING_CLOSING_QUOTATION,
    UNSUPPORTED_CONTROL_CHARCTER,
    UNSUPPORTED_ESCAPE_SEQUENCE,
};

typedef enum lexer_failures_e LexerFailureCode;


Maybe(Token) next(Input *input);

const char *lexer_strerror(LexerFailureCode code);
