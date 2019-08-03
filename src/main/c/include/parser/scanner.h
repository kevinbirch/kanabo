#pragma once

#include "location.h"
#include "str.h"

#include "parser.h"

void scanner_next(Parser *self);
void scanner_reset(Parser *self);

String *scanner_extract_lexeme(Parser *self, Location location);
