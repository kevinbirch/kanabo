#pragma once

#include "parser/input.h"
#include "parser/token.h"
#include "parser/errors.h"

typedef void (*ErrorHandler)(Position position, ParserErrorCode code);

struct lexer_s
{
    ErrorHandler callback;
    Token        current;
    Input        input;
};

typedef struct lexer_s Lexer;

Lexer *make_lexer(const char *data, size_t length);
int    lexer_init(Lexer *self, const char *data, size_t length);
void   dispose_lexer(Lexer *self);

void next(Lexer *lexer);
#define position(LEXER) (LEXER)->input.position
void reset(Lexer *lexer);
