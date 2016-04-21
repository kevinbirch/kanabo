
#include "conditions.h"

#include "parser.h"
#include "parser/location.h"


MaybeSyntaxNode parse(Parser *parser, Input *input)
{
    SyntaxNode *root = make_syntax_node(CST_ROOT, NULL, location_from_input(input));
    if(NULL == root)
    {
        return nothing_node(ERR_PARSER_OUT_OF_MEMORY);
    }
    return bind(parser, just_node(root), input);
    // free root node on error
    // return root node
    /* if(input_has_more(input)) */
    /* { */
    /*     result = path_error(ERR_PARSER_UNEXPECTED_VALUE, input_position(input)); */
    /*     goto cleanup; */
    /* } */
}
