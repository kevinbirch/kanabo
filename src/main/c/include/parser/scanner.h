#pragma once

#include "location.h"
#include "str.h"

#include "parser.h"

void scanner_next(Parser *self);
void scanner_reset(Parser *self);

String *scanner_extract_lexeme(Parser *self, Location location);

#define scanner_add_error(SELF, CODE) parser_add_error_at((SELF), (CODE), location(SELF), (SELF)->input->position.index)
#define scanner_add_error_at(SELF, CODE, LOC, IDX) parser_add_error_at((SELF), (CODE), (LOC), (IDX))
