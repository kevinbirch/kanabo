
#pragma once


#include "parser.h"


enum parser_kind
{
    RULE,
    CHOICE,
    SEQUENCE,
    OPTION,
    REPETITION,
    REFERENCE,
    LITERAL,
    NUMBER,
    INTEGER,
    TERM
};

typedef enum parser_kind ParserKind;

typedef Maybe (*parser_delegate)(Parser *parser, Input *input);
typedef void (*parser_disposer)(Parser *self);

struct parser_s
{
    ParserKind kind;
    struct
    {
        parser_disposer free;
        parser_delegate delegate;
    } vtable;
    char *repr;
};

#define ensure_more_input(INPUT) if(!input_has_more((INPUT)))  \
    {                                                          \
        return fail(ERR_PARSER_END_OF_INPUT);                  \
    }

Parser *make_parser(ParserKind kind, parser_delegate delegate);
Parser *parser_init(Parser *self, ParserKind kind, parser_delegate delegate);

void parser_destructor(void *each);

ParserKind  parser_kind(Parser *self);
const char *parser_name(Parser *self);
const char *parser_repr(Parser *self);

Maybe parser_execute(Parser *self, Input *input);
