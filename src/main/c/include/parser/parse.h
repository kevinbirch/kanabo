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
void add_internal_error(Parser *self, const char *restrict filename, int line, const char * restrict fmt, ...);
JsonPath recognize(Parser *parser);
char *unescape(const char *lexeme);
