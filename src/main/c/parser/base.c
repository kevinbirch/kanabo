
#include "log.h"

#include "parser/base.h"


static const char * const PARSER_NAMES[] =
{
    "rule",
    "choice",
    "sequence",
    "option",
    "repetition",
    "reference",
    "literal",
    "number",
    "integer",
    "signed integer",
    "non zero signed integer",
    "string"
};


static void base_free(Parser *self)
{
    free(self);
}

static MaybeSyntaxNode base_delegate(Parser *self __attribute__((unused)),
                                     MaybeSyntaxNode node,
                                     Input *input __attribute__((unused)))
{
    return node;
}

Parser *make_parser(enum parser_kind kind)
{
    Parser *parser = (Parser *)calloc(1, sizeof(Parser));
    return parser_init(parser, kind);
}

Parser *parser_init(Parser *self, enum parser_kind kind)
{
    if(NULL == self)
    {
        return NULL;
    }
    self->kind = kind;
    self->vtable.free = base_free;
    self->vtable.delegate = base_delegate;

    return self;
}

void parser_free(Parser *self)
{
    if(NULL == self)
    {
        return;
    }
    self->vtable.free(self);
    free(self);
}

void parser_destructor(void *each)
{
    parser_free((Parser *)each);
}

enum parser_kind parser_kind(Parser *self)
{
    return self->kind;
}

const char *parser_name(Parser *self)
{
    return PARSER_NAMES[parser_kind(self)];
}

const char *parser_repr(Parser *self)
{
    return NULL != self->repr ? (const char *)self->repr : parser_name(self);
}
