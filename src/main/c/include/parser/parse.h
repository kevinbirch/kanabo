#pragma once

#include "vector.h"

#include "parser/errors.h"
#include "parser/scanner.h"

struct parser_s
{
    Vector  *errors;
    Scanner *scanner;
};

typedef struct parser_s Parser;

void add_error(Parser *self, Position position, ParserErrorCode code);
