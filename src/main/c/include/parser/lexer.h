#pragma once


#include "parser/input.h"
#include "parser/token.h"


enum lexer_error_e
{
    PREMATURE_END_OF_INPUT,
    UNSUPPORTED_CONTROL_CHARACTER,
    UNSUPPORTED_ESCAPE_SEQUENCE,
};

typedef enum lexer_error_e LexerErrorCode;

struct lexer_error_s
{
    LexerErrorCode code;
    Position position;
};

typedef struct lexer_error_s LexerError;

struct lexer_s
{
    Vector *errors;
    Token   current;
    Input   input;
};

typedef struct lexer_s Lexer;

Lexer *make_lexer(const char *data, size_t length);
void   dispose_lexer(Lexer *self);

void reset(Lexer *lexer);
void next(Lexer *lexer);
#define position(LEXER) (LEXER)->input.position

const char *lexer_strerror(LexerErrorCode code);
