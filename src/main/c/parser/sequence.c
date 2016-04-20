
#include <stdio.h>

#include "vector.h"

#include "parser/compound.h"


static MaybeSyntaxNode sequence_delegate(Parser *parser, MaybeSyntaxNode node, Input *input)
{
    ensure_more_input(input);
    CompoundParser *self = (CompoundParser *)parser;
    Location location = location_from_input(input);
    SyntaxNode *collection = make_syntax_node(CST_COLLECTION, NULL, location);
    if(NULL == collection)
    {
        return nothing_node(ERR_PARSER_OUT_OF_MEMORY);
    }
    syntax_node_add_child(value(node), collection);
    MaybeSyntaxNode maybe_collection = just_node(collection);
    
    for(size_t i = 0; i < vector_length(self->children); i++)
    {
        Parser *each = vector_get(self->children, i);
        MaybeSyntaxNode branch_result = bind(each, maybe_collection, input);
        if(is_nothing(branch_result))
        {
            return branch_result;
        }
    }

    return maybe_collection;
}

Parser *sequence_parser(Parser *one, Parser *two, ...)
{
    if(NULL == one || NULL == two)
    {
        if(NULL != one)
        {
            parser_free(one);
        }
        if(NULL != two)
        {
            parser_free(two);
        }
        return NULL;
    }

    va_list rest;
    va_start(rest, two);
    CompoundParser *self = make_compound_parser(SEQUENCE, one, two, rest);
    va_end(rest);
    if(NULL == self)
    {
        return NULL;
    }
    self->base.vtable.delegate = sequence_delegate;
    asprintf(&self->base.repr, "sequence %zd branches", vector_length(self->children));

    return (Parser *)self;
}
