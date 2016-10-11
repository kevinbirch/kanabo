
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
    "term"
};


static void base_free(Parser *self)
{
    free(self);
}


Parser *make_parser(ParserKind kind, parser_delegate delegate)
{
    Parser *parser = (Parser *)calloc(1, sizeof(Parser));
    return parser_init(parser, kind, delegate);
}


Parser *parser_init(Parser *self, ParserKind kind, parser_delegate delegate)
{
    if(NULL == self)
    {
        return NULL;
    }
    self->kind = kind;
    self->vtable.free = base_free;
    self->vtable.delegate = delegate;

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


ParserKind parser_kind(Parser *self)
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


Maybe parser_execute(Parser *self, Input *input)
{
    static size_t padding = 0;
    log_trace("parser", "%*s--> %s", (2 * padding++), "", parser_repr(self));
    Maybe result = self->vtable.delegate(self, input);
    log_trace("parser",
              "%*s%s: %s", (2 * --padding), "", parser_repr(self),
              is_nothing(result) ? "failure" : "success");

    return result;
}
