#include "parser/parse.h"

#define current(PARSER) (PARSER)->scanner->current.kind
#define token(PARSER) (PARSER)->scanner->current
#define next(PARSER) scanner_next((PARSER)->scanner)
#define position(PARSER) (PARSER)->scanner->current.location.position
#define lexeme(PARSER) scanner_extract_lexeme((PARSER)->scanner, (PARSER)->scanner->current.location)

static void expect(Parser *self, TokenKind kind)
{
    bool squaked = false;

    while(kind != current(self))
    {
        if(END_OF_INPUT == current(self))
        {
            add_error(self, position(self), PREMATURE_END_OF_INPUT);
            break;
        }
        if(!squaked)
        {
            squaked = true;
            add_error(self, position(self), UNEXPECTED_INPUT);
        }
        next(self);
    }

    next(self);
}

static void parse_predicate_expression(Parser *self, Step *step)
{
    next(self);

    Predicate *predicate = xcalloc(sizeof(Predicate));

    switch(current(self))
    {
        case ASTERISK:
            predicate->kind = WILDCARD;
            break;
        case INTEGER_LITERAL:
            // parse_indexed_predicate(parent, self);
            break;
        case COLON:
            // parse_slice_predicate(parent, self);
            break;
        case AT:
        case DOLLAR:
        case DOT:
        case DOT_DOT:
        case NAME:
            // parse_join_predicate(parent, self);
            break;
        case END_OF_INPUT:
            add_error(self, position(self), EXPECTED_PREDICATE_EXPRESSION_PRODUCTION);
            return;
        default:
            add_error(self, position(self), EXPECTED_PREDICATE_EXPRESSION_PRODUCTION);
            // N.B. - keep going until the predicate closing `]`
            break;
    }

    expect(self, CLOSE_BRACKET);

    step->predicate = predicate;
}

static void parse_predicate(Parser *self, Step *step)
{
    if(current(self) == OPEN_BRACKET)
    {
        parse_predicate_expression(self, step);
    }
    else if(current(self) == OPEN_FILTER)
    {
        // parse_filter_expression(parent, self)
    }
}

static void parse_step(Parser *self, Step *step)
{
    switch(current(self))
    {
        case QUOTED_NAME:
            char *raw = lexeme(self);
            if(NULL == raw)
            {
                add_internal_error(self, __FILE__, __LINE__, "can't extract lexeme at %zu:%zu", token(self).location.index, token(self).location.extent);
                break;
            }
            // xxx - strip the lexeme
            char *cooked = unescape(raw);
            free(raw);
            step->test.name.value = (uint8_t *)cooked;            
            step->test.name.length = strlen(cooked);
            break;
        case NAME:
            step->test.name.length = token(self).location.extent;
            step->test.name.value = (uint8_t *)lexeme(self);
            break;
        /*
        case EQUALS:
            // transformer
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
            add_error(self, position(self), EXPECTED_STEP_PRODUCTION);
            return;
        default:
            add_error(self, position(self), EXPECTED_STEP_PRODUCTION);
            // xxx - enter recovery mode
            break;
    }

    next(self);
    parse_predicate(self, step);
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

static void parse_recursive_step(Parser *self, JsonPath *path)
{
    if(DOT_DOT == current(self))
    {
        next(self);
    }

    parse_step(self, make_step(RECURSIVE, path));
}

static void parse_relative_step(Parser *self, JsonPath *path)
{
    if(DOT == current(self))
    {
        next(self);
    }
    
    parse_step(self, make_step(SINGLE, path));
}

static void parse_qualified_head_step(Parser *self, JsonPath *path)
{
    Step *step = xcalloc(sizeof(Step));
    step->kind = ROOT;
    step->test.kind = NAME_TEST;

    path->length++;
    vector_add(path->steps, step);

    next(self);
    parse_predicate(self, step);
}

static void parse_head_step(Parser *self, JsonPath *path)
{
    switch(current(self))
    {
        case DOLLAR:
            path->kind = ABSOLUTE_PATH;
            parse_qualified_head_step(self, path);
            break;
        case AT:
            path->kind = RELATIVE_PATH;
            parse_qualified_head_step(self, path);
            break;
        case END_OF_INPUT:
            break;
        default:
            path->kind = RELATIVE_PATH;
            parse_relative_step(self, path);
            break;
    }
}

JsonPath recognize(Parser *self)
{
    JsonPath path;
    path.steps = make_vector_with_capacity(1);

    next(self);
    parse_head_step(self, &path);

    bool done = false;
    while(!done)
    {
        switch(current(self))
        {
            case DOT:
                parse_relative_step(self, &path);
                break;
            case DOT_DOT:
                parse_recursive_step(self, &path);
                break;
            case END_OF_INPUT:
                done = true;
                break;
            default:
                add_error(self, position(self), EXPECTED_QUALIFIED_STEP_PRODUCTION);
                done = true;
                break;
        }
    }

    return path;
}
