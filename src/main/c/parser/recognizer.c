#include "parser/parse.h"

#define current(PARSER) (PARSER)->scanner->current.kind
#define token(PARSER) (PARSER)->scanner->current
#define next(PARSER) scanner_next((PARSER)->scanner)
#define position(PARSER) (PARSER)->scanner->current.location.position
#define lexeme(PARSER) scanner_extract_lexeme((PARSER)->scanner, (PARSER)->scanner->current.location)

static void expect(Parser *parser, TokenKind kind)
{
    bool squaked = false;

    while(kind != current(parser))
    {
        if(END_OF_INPUT == current(parser))
        {
            add_error(parser, position(parser), PREMATURE_END_OF_INPUT);
            break;
        }
        if(!squaked)
        {
            squaked = true;
            add_error(parser, position(parser), UNEXPECTED_INPUT);
        }
        next(parser);
    }

    next(parser);
}

#include <stdio.h>

static void parse_predicate_expression(Step *step, Parser *parser)
{
    next(parser);

    Predicate *predicate = xcalloc(sizeof(Predicate));

    switch(current(parser))
    {
        case ASTERISK:
            predicate->kind = WILDCARD;
            break;
        case INTEGER_LITERAL:
            // parse_indexed_predicate(parent, parser);
            break;
        case COLON:
            // parse_slice_predicate(parent, parser);
            break;
        case AT:
        case DOLLAR:
        case DOT:
        case DOT_DOT:
        case NAME:
            printf("found join\n");
            // parse_join_predicate(parent, parser);
            break;
        case END_OF_INPUT:
            add_error(parser, position(parser), EXPECTED_PREDICATE_EXPRESSION_PRODUCTION);
            return;
        default:
            add_error(parser, position(parser), EXPECTED_PREDICATE_EXPRESSION_PRODUCTION);
            // N.B. - keep going until the predicate closing `]`
            break;
    }

    expect(parser, CLOSE_BRACKET);

    step->predicate = predicate;
}

static void parse_predicate(Step *step, Parser *parser)
{
    next(parser);
    if(current(parser) == OPEN_BRACKET)
    {
        parse_predicate_expression(step, parser);
    }
    else if(current(parser) == OPEN_FILTER)
    {
        // parse_filter_expression(parent, parser)
    }
}

static void parse_step(Parser *parser, Step *step)
{
    switch(current(parser))
    {
        case QUOTED_NAME:
            char *raw = lexeme(parser);
            char *cooked = unescape(raw);
            free(raw);
            step->test.name.value = (uint8_t *)cooked;            
            step->test.name.length = strlen(cooked);
            break;
        case NAME:
            step->test.name.length = token(parser).location.extent;
            step->test.name.value = (uint8_t *)lexeme(parser);
            break;
        /*
        case EQUALS:
            // parse transformer
            break;
        case EXCLAMATION:
            // tag selector
            break;
        case AMPERSAND:
            // anchor selector
            break;
        */
        case OBJECT_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = OBJECT_TEST;
            break;
        case ARRAY_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = ARRAY_TEST;
            break;
        case STRING_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = STRING_TEST;
            break;
        case NUMBER_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = NUMBER_TEST;
            break;
        /*
        case INTEGER_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = INTEGER_TEST;
            break;
        case DECIMAL_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = DECIMAL_TEST;
            break;
        case TIMESTAMP_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = TIMESTAMP_TEST;
            break;
        */
        case BOOLEAN_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = BOOLEAN_TEST;
            break;
        case NULL_SELECTOR:
            step->test.kind = TYPE_TEST;
            step->test.type = NULL_TEST;
            break;
        case END_OF_INPUT:
            add_error(parser, position(parser), EXPECTED_STEP_PRODUCTION);
            return;
        default:
            add_error(parser, position(parser), EXPECTED_STEP_PRODUCTION);
            // xxx - enter recovery mode
            break;
    }

    parse_predicate(step, parser);
}

static inline Step *make_step(enum step_kind kind, JsonPath *path)
{
    Step *step = xcalloc(sizeof(Step));
    step->kind = kind;
    step->test.kind = NAME_TEST;

    path->length++;
    vector_add(path->steps, step);

    return step;
}

static void parse_recursive_step(Parser *parser, JsonPath *path)
{
    if(DOT_DOT == current(parser))
    {
        next(parser);
    }

    parse_step(parser, make_step(RECURSIVE, path));
}

static void parse_relative_step(Parser *parser, JsonPath *path)
{
    if(DOT == current(parser))
    {
        next(parser);
    }
    
    parse_step(parser, make_step(SINGLE, path));
}

static void parse_qualified_head_step(Parser *parser, JsonPath *path)
{
    Step *step = xcalloc(sizeof(Step));
    step->kind = ROOT;
    step->test.kind = NAME_TEST;

    path->length++;
    vector_add(path->steps, step);

    parse_predicate(step, parser);
}

static void parse_head_step(Parser *parser, JsonPath *path)
{
    switch(current(parser))
    {
        case DOLLAR:
            path->kind = ABSOLUTE_PATH;
            parse_qualified_head_step(parser, path);
            break;
        case AT:
            path->kind = RELATIVE_PATH;
            parse_qualified_head_step(parser, path);
            break;
        case END_OF_INPUT:
            break;
        default:
            path->kind = RELATIVE_PATH;
            parse_relative_step(parser, path);
            break;
    }
}

JsonPath recognize(Parser *parser)
{
    JsonPath path;
    path.steps = make_vector_with_capacity(1);

    next(parser);
    parse_head_step(parser, &path);

    bool done = false;
    while(!done)
    {
        switch(current(parser))
        {
            case DOT:
                parse_relative_step(parser, &path);
                break;
            case DOT_DOT:
                parse_recursive_step(parser, &path);
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
