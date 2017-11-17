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
    // N.B. - eat the opening bracket
    next(self);

    Predicate *predicate = xcalloc(sizeof(Predicate));
    step->predicate = predicate;

    switch(current(self))
    {
        case ASTERISK:
            predicate->kind = WILDCARD;
            next(self);
            break;
        case INTEGER_LITERAL:
            // xxx - parse_indexed_predicate(parent, self);
            break;
        case COLON:
            // xxx - parse_slice_predicate(parent, self);
            break;
        case AT:
        case DOLLAR:
        case DOT:
        case DOT_DOT:
        case NAME:
            // xxx - parse_join_predicate(parent, self);
            add_error(self, position(self), EXPECTED_PREDICATE_PRODUCTION);
            next(self);
            break;
        case CLOSE_BRACKET:
            add_error(self, position(self), EXPECTED_PREDICATE_PRODUCTION);
            next(self);
            return;
        case END_OF_INPUT:
            add_error(self, position(self), EXPECTED_PREDICATE_PRODUCTION);
            return;
        default:
            add_error(self, position(self), EXPECTED_PREDICATE_PRODUCTION);
            next(self);
            // N.B. - keep going until the predicate closing `]`
            break;
    }

    expect(self, CLOSE_BRACKET);
}

static void parse_predicate(Parser *self, Step *step)
{
    if(OPEN_BRACKET == current(self))
    {
        parse_predicate_expression(self, step);
    }
    else if(OPEN_FILTER == current(self))
    {
        // parse_filter_expression(parent, self)
    }
}

static void parse_quoted_name(Parser *self, Step *step)
{
    Location loc = self->scanner->current.location;
    char *raw = lexeme(self);
    if(NULL == raw)
    {
        add_internal_error(self, __FILE__, __LINE__, "can't extract lexeme at %zu:%zu", loc.index, loc.extent);
        return;
    }
    if(strlen(raw) < 2)
    {
        // N.B. - short read on token
        goto cleanup;
    }

    // N.B. - trim leading quote
    char *cooked = unescape(raw + 1);
    if(NULL == cooked)
    {
        add_error(self, position(self), UNSUPPORTED_ESCAPE_SEQUENCE);
        goto cleanup;
    }

    size_t length = strlen(cooked);
    if('\'' == cooked[length-1])
    {
        length--;
    }
    step->test.name.value = (uint8_t *)cooked;
    step->test.name.length = length;

  cleanup:
    free(raw);
}

static void recover(Parser *self, Step *step)
{
    bool done = false;

    next(self);

    while(!done)
    {
        switch(current(self))
        {
            case OPEN_BRACKET:
            case OPEN_FILTER:
                done = true;
                parse_predicate(self, step);
                break;
            case DOT:
            case DOT_DOT:
            case END_OF_INPUT:
                done = true;
                break;
            default:
                next(self);
                break;
        }
    }
}

static void parse_step(Parser *self, Step *step)
{
    switch(current(self))
    {
        case QUOTED_NAME:
            parse_quoted_name(self, step);
            break;
        case NAME:
            step->test.name.length = token(self).location.extent;
            step->test.name.value = (uint8_t *)lexeme(self);
            break;
        /*
        case EQUALS:
            // xxx - transformer
            break;
        case EXCLAMATION:
            // xxx - tag selector
            break;
        case AMPERSAND:
            // xxx - anchor selector
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
            // xxx - type selector
            step->test.kind = TYPE_TEST;
            step->test.type = INTEGER_TEST;
            break;
        case DECIMAL_SELECTOR:
            // xxx - type selector
            step->test.kind = TYPE_TEST;
            step->test.type = DECIMAL_TEST;
            break;
        case TIMESTAMP_SELECTOR:
            // xxx - type selector
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
            recover(self, step);
            return;
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
