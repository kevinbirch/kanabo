#pragma once

#include "parser.h"

JsonPath *recognize(Parser *parser);

#define parser_add_error(SELF, CODE) parser_add_error_at((SELF), (CODE), location(SELF), 0)

#define VAL(x) #x
#define STR(x) VAL(x)

void parser_add_internal_error_at(Parser *self, const char *restrict location, const char * restrict fmt, ...);
#define parser_add_internal_error(SELF, FMT, ...) parser_add_internal_error_at((SELF), __FILE__":"STR(__LINE__), (FMT), ##__VA_ARGS__)
