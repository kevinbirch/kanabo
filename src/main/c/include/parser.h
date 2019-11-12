#pragma once

#include "input.h"
#include "location.h"
#include "maybe.h"
#include "str.h"
#include "vector.h"

#include "jsonpath.h"

enum token_kind_e
{
    START,
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
    OPEN_PARENTHESIS,
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

typedef enum token_kind_e TokenKind;

struct token_s
{
    TokenKind kind;
    Location  location;
};

typedef struct token_s Token;

struct parser_s
{
    Vector  *errors;
    Token    current;
    Input   *input;
};

typedef struct parser_s Parser;

enum parser_error_e
{
    INTERNAL_ERROR,
    EMPTY_INPUT,
    PREMATURE_END_OF_INPUT,
    UNCLOSED_QUOTATION,
    UNBALANCED_PRED_DELIM,
    UNSUPPORTED_CONTROL_CHARACTER,
    UNSUPPORTED_ESCAPE_SEQUENCE,
    UNSUPPORTED_UNICODE_SEQUENCE,
    UNEXPECTED_INPUT,
    EXPECTED_QUALIFIED_STEP_PRODUCTION,
    EXPECTED_STEP_PRODUCTION,
    EXPECTED_PREDICATE_PRODUCTION,
    EXPECTED_INTEGER,
    INTEGER_TOO_BIG,
    INTEGER_TOO_SMALL,
    STEP_CANNOT_BE_ZERO,
};

typedef enum parser_error_e ParserErrorCode;

struct parser_error_s
{
    ParserErrorCode code;
    Position        position;
};

typedef struct parser_error_s ParserError;

struct internal_error_s
{
    union
    {
        ParserError base;
        struct parser_error_s;
    };
    const char *location;
    String     *message;
};

typedef struct internal_error_s ParserInternalError;

defmaybep_error(JsonPath, Vector *);

#define location(SELF) (SELF)->current.location
#define position(SELF) (SELF)->current.location.position

const char *token_name(TokenKind kind);

Maybe(JsonPath) parse(const char *expression);

#define VAL(x) #x
#define STR(x) VAL(x)

void parser_add_error_at(Parser *self, ParserErrorCode code, Position position);
#define parser_add_error(SELF, CODE) parser_add_error_at((SELF), (CODE), position(SELF))
void parser_add_internal_error_at(Parser *self, const char *restrict location, const char * restrict fmt, ...);
#define parser_add_internal_error(SELF, FMT, ...) parser_add_internal_error_at((SELF), __FILE__":"STR(__LINE__), (FMT), ##__VA_ARGS__)

const char *parser_strerror(ParserErrorCode code);
void parser_dispose_errors(Vector *errors);
void parser_release(Parser *self);
