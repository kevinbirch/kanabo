#pragma once

#include "vector.h"

#include "jsonpath.h"
#include "parser.h"
#include "parser/scanner.h"

struct parser_s
{
    Vector  *errors;
    Scanner *scanner;
};

typedef struct parser_s Parser;

void add_error(Parser *self, Position position, ParserErrorCode code);
JsonPath recognize(Parser *parser);
char *unescape(const char *lexeme);
