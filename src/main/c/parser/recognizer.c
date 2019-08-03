#include <errno.h>
#include <stdint.h>

#include "xalloc.h"

#include "parser/escape.h"
#include "parser/recognizer.h"
#include "parser/scanner.h"

#define current_kind(SELF) (SELF)->current.kind
#define lexeme_at(PARSER, LOC) scanner_extract_lexeme((PARSER), (LOC))
#define lexeme(PARSER) lexeme_at((PARSER), location(PARSER))
#define parser_add_lexeme_error(SELF, LOC) parser_add_internal_error((SELF), "can't extract lexeme at %zu:%zu", (LOC).index, (LOC).extent)

static inline void expect(Parser *self, TokenKind kind)
{
    bool squaked = false;

    while(kind != current_kind(self))
    {
        if(END_OF_INPUT == current_kind(self))
        {
            parser_add_error(self, PREMATURE_END_OF_INPUT);
            break;
        }
        if(!squaked)
        {
            squaked = true;
            parser_add_error(self, UNEXPECTED_INPUT);
        }
        scanner_next(self);
    }
}

static inline int64_t parse_index(Parser *self)
{
    Position start = position(self);
    int64_t index = -1;

    bool negate = false;
    if(MINUS == current_kind(self))
    {
        negate = true;
        scanner_next(self);

        if(INTEGER_LITERAL != current_kind(self))
        {
            parser_add_error_at(self, EXPECTED_INTEGER, start);
            goto end;
        }
    }
    
    String *raw = lexeme(self);
    scanner_next(self);

    if(NULL == raw)
    {
        parser_add_lexeme_error(self, location(self));
        goto end;
    }

    errno = 0;
    index = strtoll(C(raw), NULL, 10);
    if(ERANGE == errno && negate)
    {
        parser_add_error_at(self, INTEGER_TOO_SMALL, start);
    }
    else if(ERANGE == errno)
    {
        parser_add_error_at(self, INTEGER_TOO_BIG, start);
    }

    if(negate)
    {
        index = -index;
    }

    free(raw);
  end:
    return index;
}

static void parse_slice_predicate(Parser *self, Predicate *predicate)
{
    predicate->kind = SLICE;

    if(INTEGER_LITERAL == current_kind(self) || MINUS == current_kind(self))
    {
        predicate->slice.specified |= SLICE_FROM;
        predicate->slice.from = parse_index(self);
    }

    if(COLON != current_kind(self))
    {
        parser_add_error(self, UNEXPECTED_INPUT);
        return;
    }

    // N.B. - eat the colon
    scanner_next(self);

    if(INTEGER_LITERAL == current_kind(self) || MINUS == current_kind(self))
    {
        predicate->slice.specified |= SLICE_TO;
        predicate->slice.to = parse_index(self);
    }

    if(COLON == current_kind(self))
    {
        scanner_next(self);
        if(INTEGER_LITERAL == current_kind(self) || MINUS == current_kind(self))
        {
            predicate->slice.specified |= SLICE_STEP;
            Position start = position(self);
            predicate->slice.step = parse_index(self);
            if(0 == predicate->slice.step)
            {
                parser_add_error_at(self, STEP_CANNOT_BE_ZERO, start);
            }
        }
    }
}

static void parse_indexed_predicate(Parser *self, Predicate *predicate)
{
    int64_t index = parse_index(self);

    if(COLON == current_kind(self))
    {
        predicate->slice.specified |= SLICE_FROM;
        predicate->slice.from = index;
        parse_slice_predicate(self, predicate);
        return;
    }

    predicate->kind = SUBSCRIPT;
    predicate->subscript.index = index;
}

static void parse_predicate_expression(Parser *self, Step *step)
{
    Position start = position(self);

    // N.B. - eat the opening bracket
    scanner_next(self);

    Predicate *predicate = xcalloc(sizeof(Predicate));
    step->predicate = predicate;

    switch(current_kind(self))
    {
        case ASTERISK:
            predicate->kind = WILDCARD;
            scanner_next(self);
            break;
        case MINUS:
        case INTEGER_LITERAL:
            parse_indexed_predicate(self, predicate);
            break;
        case COLON:
            parse_slice_predicate(self, predicate);
            break;
        case AT:
        case DOLLAR:
        case DOT:
        case DOT_DOT:
        case NAME:
            // xxx - parse_join_predicate(parent, self);
            parser_add_error(self, EXPECTED_PREDICATE_PRODUCTION);
            scanner_next(self);
            break;
        case CLOSE_BRACKET:
            parser_add_error(self, EXPECTED_PREDICATE_PRODUCTION);
            scanner_next(self);
            return;
        case END_OF_INPUT:
            break;
        default:
            parser_add_error(self, EXPECTED_PREDICATE_PRODUCTION);
            scanner_next(self);
            // N.B. - keep going until the predicate closing `]`
            break;
    }

    expect(self, CLOSE_BRACKET);

    if(CLOSE_BRACKET != current_kind(self))
    {
        parser_add_error_at(self, UNBALANCED_PRED_DELIM, start);
    }

    scanner_next(self);
}

static void parse_predicate(Parser *self, Step *step)
{
    if(OPEN_BRACKET == current_kind(self))
    {
        parse_predicate_expression(self, step);
    }
    else if(OPEN_FILTER == current_kind(self))
    {
        // parse_filter_expression(parent, self)
    }
}

static void parse_quoted_name(Parser *self, Step *step)
{
    Location loc = location(self);
    Location unquoted = 
        {
            // N.B. - trim leading and trailing quotes
            .index = loc.index + 1,
            .line = loc.line,
            .offset = loc.offset + 1,
            .extent = loc.extent - 2
        };
        
    String *raw = lexeme_at(self, unquoted);
    if(NULL == raw)
    {
        parser_add_lexeme_error(self, unquoted);
        return;
    }

    if(strlen(raw) < 2)
    {
        step->test.name = raw;
        return;
    }

    String *cooked = unescape(self, raw);
    step->test.name = cooked;

    dispose_string(raw);
}

static void recover(Parser *self, Step *step)
{
    scanner_next(self);

    while(true)
    {
        switch(current_kind(self))
        {
            case OPEN_BRACKET:
            case OPEN_FILTER:
                parse_predicate(self, step);
                return;
            case DOT:
            case DOT_DOT:
            case END_OF_INPUT:
                return;
            default:
                scanner_next(self);
                break;
        }
    }
}

static void parse_step(Parser *self, Step *step)
{
    step->location = location(self);

    switch(current_kind(self))
    {
        case ASTERISK:
            step->test.kind = WILDCARD_TEST;
            break;
        case QUOTED_NAME:
            parse_quoted_name(self, step);
            break;
        case NAME:
            step->test.name = lexeme(self);
            if(NULL == step->test.name)
            {
                parser_add_lexeme_error(self, step->location);
                return;
            }
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
            parser_add_error(self, EXPECTED_STEP_PRODUCTION);
            return;
        default:
            parser_add_error(self, EXPECTED_STEP_PRODUCTION);
            recover(self, step);
            return;
    }

    scanner_next(self);
    parse_predicate(self, step);
}

static inline Step *make_step(enum step_kind kind, JsonPath *path)
{
    Step *step = xcalloc(sizeof(Step));
    step->kind = kind;
    step->test.kind = NAME_TEST;

    vector_add(path->steps, step);

    return step;
}

static void parse_recursive_step(Parser *self, JsonPath *path)
{
    if(DOT_DOT == current_kind(self))
    {
        scanner_next(self);
    }

    parse_step(self, make_step(RECURSIVE, path));
}

static void parse_relative_step(Parser *self, JsonPath *path)
{
    if(DOT == current_kind(self))
    {
        scanner_next(self);
    }
    
    parse_step(self, make_step(SINGLE, path));
}

static void parse_qualified_head_step(Parser *self, JsonPath *path)
{
    Step *step = xcalloc(sizeof(Step));
    step->kind = ROOT;
    step->test.kind = NAME_TEST;
    step->location = location(self);

    vector_add(path->steps, step);

    scanner_next(self);
    parse_predicate(self, step);
}

static void parse_head_step(Parser *self, JsonPath *path)
{
    switch(current_kind(self))
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

JsonPath *recognize(Parser *self)
{
    JsonPath *path = make_jsonpath(RELATIVE_PATH);

    scanner_next(self);
    parse_head_step(self, path);

    bool done = false;
    while(!done)
    {
        switch(current_kind(self))
        {
            case DOT:
                parse_relative_step(self, path);
                break;
            case DOT_DOT:
                parse_recursive_step(self, path);
                break;
            case END_OF_INPUT:
                done = true;
                break;
            default:
                parser_add_error(self, EXPECTED_QUALIFIED_STEP_PRODUCTION);
                done = true;
                break;
        }
    }

    return path;
}
