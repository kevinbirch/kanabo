#include "parser/recognizer.h"

#define current(PARSER) (PARSER)->scanner->current.kind
#define token(PARSER) (PARSER)->scanner->current
#define next(PARSER) scanner_next((PARSER)->scanner)
#define position(PARSER) (PARSER)->scanner->current.location.position

static inline AstNode *add_child(AstNode *parent, AstNodeKind kind, Token token)
{
    AstNode *node = make_ast_node(kind, token);
    node_add_child(parent, node);

    return node;
}

static bool parse_predicate_expression(AstNode *parent, Parser *parser)
{
    next(parser);
    switch(current(parser))
    {
        case ASTERISK:
            add_child(parent, AST_WILDCARD, token(parser));
            break;
        case INTEGER_LITERAL:
            // parse_indexed_predicate(parent, parser);
            break;
        case COLON:
            // parse_slice_predicate(parent, parser);
            break;
        case AT:
        case NAME:
            // parse_join_predicate(parent, parser);
            break;
        case END_OF_INPUT:
            add_error(parser, position(parser), EXPECTED_PREDICATE_EXPRESSION_PRODUCTION);
            return false;
        default:
            add_error(parser, position(parser), EXPECTED_PREDICATE_EXPRESSION_PRODUCTION);
            // N.B. - keep going until the predicate closing `]`
            break;
    }

    // expect(CLOSE_BRACKET);

    return true;
}

static bool parse_predicate(AstNode *parent, Parser *parser)
{
    next(parser);
    if(current(parser) == OPEN_BRACKET)
    {
        return parse_predicate_expression(parent, parser);
    }
    else if(current(parser) == OPEN_FILTER)
    {
        // parse_filter_expression(parent, parser)
    }

    return true;
}

static inline bool parse_explicit_head_step(AstNode *parent, AstNodeKind kind, Parser *parser)
{
    AstNode *step = add_child(parent, kind, token(parser));
    if(NULL == step)
    {
        return false;
    }

    return parse_predicate(step, parser);
}

static bool parse_relative_step(AstNode *parent, Parser *parser)
{
    AstNode *step = add_child(parent, AST_RELATIVE_STEP, token(parser));
    // xxx - insert synthetic `.` node if current is something else?
    // xxx - unqualified relative step ast node kind?

    return true;
}

static bool parse_head_step(AstNode *parent, Parser *parser)
{
    switch(current(parser))
    {
        case DOLLAR:
            return parse_explicit_head_step(parent, AST_ROOT_STEP, parser);
            break;
        case AT:
            return parse_explicit_head_step(parent, AST_RELATIVE_HEAD_STEP, parser);
            break;
        case END_OF_INPUT:
            return false;
            break;
        default:
            return parse_relative_step(parent, parser);
    }
}

JsonPath recognize(Parser *parser)
{
    JsonPath path;

    next(parser);
    if(!parse_head_step(path, parser))
    {
        goto end;
    }

    bool done = false;
    while(!done)
    {
        switch(current(parser))
        {
            case DOT:
                // parse_relative_step(path, parser);
                break;
            case DOT_DOT:
                // parse_recursive_step(path, parser);
                break;
            case END_OF_INPUT:
                done = true;
                break;
            default:
                add_error(parser, position(parser), EXPECTED_QUALIFIED_STEP_PRODUCTION);
                done = true;
                break;
        }
        next(parser);
    }

    return path;
}
