#pragma once

#include "parser/input.h"
#include "parser/token.h"
#include "parser/errors.h"

typedef void (*ErrorHandler)(Position position, ParserErrorCode code);

struct scanner_s
{
    ErrorHandler callback;
    Token        current;
    Input        input;
};

typedef struct scanner_s Scanner;

Scanner *make_scanner(const char *data, size_t length);
void     dispose_scanner(Scanner *self);

void next(Scanner *scanner);
#define position(SCANNER) (SCANNER)->input.position
void reset(Scanner *scanner);
