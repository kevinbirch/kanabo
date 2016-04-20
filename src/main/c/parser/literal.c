
#include <stdio.h>

#include "parser/location.h"
#include "parser/base.h"


struct literal_parser_s
{
    Parser   base;
    String   *value;
};

typedef struct literal_parser_s LiteralParser;


static MaybeSyntaxNode literal_delegate(Parser *parser, MaybeSyntaxNode node, Input *input)
{
    LiteralParser *self = (LiteralParser *)parser;

    ensure_more_input(input);
    input_skip_whitespace(input);
    if(input_consume_if(input, self->value))
    {
        Location location = location_from_input(input);
        SyntaxNode *literal = make_syntax_node(CST_LITERAL, self->value, location);
        syntax_node_add_child(value(node), literal);

        return just_node(literal);
    }
    else
    {
        return nothing_node(ERR_PARSER_UNEXPECTED_VALUE);
    }
}

Parser *literal(const char *value)
{
    if(NULL == value)
    {
        return NULL;
    }
    LiteralParser *self = calloc(1, sizeof(LiteralParser));
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, LITERAL);
    self->base.vtable.delegate = literal_delegate;
    asprintf(&self->base.repr, "literal '%s'", value);
    self->value = make_string(value);

    return (Parser *)self;
}
