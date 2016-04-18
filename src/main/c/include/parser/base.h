
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
    SIGNED_INTEGER,
    NON_ZERO_SIGNED_INTEGER,
    TERM
};

struct parser_s
{
    enum parser_kind kind;
    struct
    {
        void (*free)(Parser *self);
        MaybeSyntaxNode (*delegate)(Parser *parser, MaybeSyntaxNode node, Input *input);
    } vtable;
    char *repr;
};

#define ensure_more_input(INPUT) if(!has_more((INPUT)))  \
    {                                                    \
        return nothing_node(ERR_PARSER_END_OF_INPUT);    \
    }

Parser *make_parser(enum parser_kind kind);
Parser *parser_init(Parser *self, enum parser_kind kind);

void parser_destructor(void *each);

enum parser_kind parser_kind(Parser *self);
const char *parser_name(Parser *self);
const char *parser_repr(Parser *self);
bool is_terminal(Parser *self);
bool is_nonterminal(Parser *self);
