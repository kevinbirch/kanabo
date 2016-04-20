
#include <stdio.h>

#include "parser/wrapped.h"


static MaybeSyntaxNode repetition_delegate(Parser *parser, MaybeSyntaxNode node, Input *input)
{
    ensure_more_input(input);
    WrappedParser *self = (WrappedParser *)parser;
    Location location = location_from_input(input);
    SyntaxNode *collection = make_syntax_node(CST_COLLECTION, NULL, location);
    if(NULL == collection)
    {
        return nothing_node(ERR_PARSER_OUT_OF_MEMORY);
    }
    syntax_node_add_child(value(node), collection);
    MaybeSyntaxNode maybe_collection = just_node(collection);
 
    MaybeSyntaxNode branch_result = bind(self->child, maybe_collection, input);
    if(ERR_PARSER_END_OF_INPUT == branch_result.code)
    {
        return branch_result;
    }
    while(is_value(branch_result))
    {
        branch_result = bind(self->child, maybe_collection, input);
    }

    return maybe_collection;
}

Parser *repetition(Parser *expression)
{
    if(NULL == expression)
    {
        return NULL;
    }

    WrappedParser *self = make_wrapped_parser(REPETITION, expression);
    if(NULL == self)
    {
        return NULL;
    }
    self->base.vtable.delegate = repetition_delegate;
    asprintf(&self->base.repr, "repetition of %s", parser_repr(expression));

    return (Parser *)self;
}
