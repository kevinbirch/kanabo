#pragma once


#include "maybe.h"

#include "parser/location.h"


enum token_kind
{
    END_OF_INPUT,
    DOLLAR,
    AT,
    DOT_DOT,
    DOT,
    EQUALS,
    COLON,
    COMMA,
    EXCLAMATION,
    AMPERSAND,
    ASTERISK,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    OPEN_BRACE,
    CLOSE_BRACE,
    OPEN_PARENTHSIS,
    CLOSE_PARENTHESIS,
    OPEN_FILTER,
    GREATER_THAN,
    GREATER_THAN_EQUAL,
    LESS_THAN,
    LESS_THAN_EQUAL,
    NOT_EQUAL,
    PLUS,
    MINUS,
    SLASH,
    PERCENT,
    OBJECT_SELECTOR,
    ARRAY_SELECTOR,
    STRING_SELECTOR,
    NUMBER_SELECTOR,
    INTEGER_SELECTOR,
    DECIMAL_SELECTOR,
    TIMESTAMP_SELECTOR,
    BOOLEAN_SELECTOR,
    NULL_SELECTOR,
    NULL_LITERAL,
    BOOLEAN_OR,
    BOOLEAN_AND,
    BOOLEAN_LITERAL_TRUE,
    BOOLEAN_LITERAL_FALSE,
    STRING_LITERAL,
    INTEGER_LITERAL,
    REAL_LITERAL,
    QUOTED_NAME,
    NAME
};

struct token_s
{
    enum token_kind kind;
    SourceLocation  lexeme;
};

typedef struct token_s Token;

make_maybe(Token);

const char *token_name(enum token_kind kind);
