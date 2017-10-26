#pragma once


#include "parser/input.h"
#include "parser/token.h"


enum lexer_failures_e
{
    PREMATURE_END_OF_INPUT,
    UNSUPPORTED_CONTROL_CHARACTER,
    UNSUPPORTED_ESCAPE_SEQUENCE,
};

typedef enum lexer_failures_e LexerFailureCode;


Maybe(Token) next(Input *input);

const char *lexer_strerror(LexerFailureCode code);
