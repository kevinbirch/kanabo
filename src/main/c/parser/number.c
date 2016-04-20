
#include "parser/base.h"


static MaybeSyntaxNode number_delegate(Parser *parser __attribute__((unused)), MaybeSyntaxNode node, Input *input)
{
    ensure_more_input(input);
    input_skip_whitespace(input);
    return node;
}

Parser *number(void)
{
    Parser *self = make_parser(NUMBER);
    if(NULL == self)
    {
        return NULL;
    }
    self->vtable.delegate = number_delegate;

    return self;
}
