#include "parser/parse.h"

#define current(PARSER) (PARSER)->scanner->current.kind
#define token(PARSER) (PARSER)->scanner->current
#define next(PARSER) scanner_next((PARSER)->scanner)
#define position(PARSER) (PARSER)->scanner->current.location.position
#define lexeme(PARSER) scanner_extract_lexeme((PARSER)->scanner, (PARSER)->scanner->current.location)

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
        case NAME:
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

    // expect(CLOSE_BRACKET);

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

static void parse_qualified_head_step(JsonPath *path, Parser *parser)
{
    Step *step = xcalloc(sizeof(Step));
    step->kind = ROOT;
    step->test.kind = NAME_TEST;

    path->length++;
    vector_add(path->steps, step);

    parse_predicate(step, parser);
}

static void parse_relative_step(JsonPath *path, Parser *parser)
{
    enum step_kind kind = SINGLE;
    if(DOT == current(parser))
    {
        next(parser);
    }
    else if(DOT_DOT == current(parser))
    {
        next(parser);
        kind = RECURSIVE;
    }

    Step *step = xcalloc(sizeof(Step));
    step->kind = kind;
    step->test.kind = NAME_TEST;

    path->length++;
    vector_add(path->steps, step);
    
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
        default:
            add_error(parser, position(parser), EXPECTED_STEP_PRODUCTION);
            // xxx - enter recovery mode
            break;
    }

    parse_predicate(step, parser);
}

static bool parse_head_step(JsonPath *path, Parser *parser)
{
    bool result = false;

    switch(current(parser))
    {
        case DOLLAR:
            path->kind = ABSOLUTE_PATH;
            parse_qualified_head_step(path, parser);
            result = true;
            break;
        case AT:
            path->kind = RELATIVE_PATH;
            parse_qualified_head_step(path, parser);
            result = true;
            break;
        case END_OF_INPUT:
            break;
        default:
            path->kind = RELATIVE_PATH;
            parse_relative_step(path, parser);
            result = true;
            break;
    }

    return result;
}

JsonPath recognize(Parser *parser)
{
    JsonPath path;
    path.steps = make_vector_with_capacity(1);

    next(parser);
    if(!parse_head_step(&path, parser))
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

  end:
    return path;
}
