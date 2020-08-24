#include <errno.h>
#include <stdint.h>

#include "xalloc.h"

#include "parser/escape.h"
#include "parser/recognizer.h"
#include "parser/scanner.h"

#define current_kind(PARSER) (PARSER)->current.kind
#define lexeme_at(PARSER, LOC) scanner_extract_lexeme((PARSER), (LOC))
#define lexeme(PARSER) lexeme_at((PARSER), location(PARSER).location)
#define scan_position(PARSER) (PARSER)->input->position

#define parser_add_lexeme_error(PARSER, LOC) parser_add_internal_error((PARSER), "can't extract lexeme at %zu:%zu", (LOC).start.index, (LOC).end.index)


static inline void expect(Parser *self, TokenKind kind)
{
    bool squawked = false;

    while(kind != current_kind(self))
    {
        if(END_OF_INPUT == current_kind(self))
        {
            parser_add_error(self, PREMATURE_END_OF_INPUT);
            break;
        }
        if(!squawked)
        {
            squawked = true;
            parser_add_error(self, UNEXPECTED_INPUT);
        }
        scanner_next(self);
    }
}

static inline int64_t parse_index(Parser *self)
{
    int64_t index = -1;

    if(INTEGER_LITERAL != current_kind(self))
    {
        parser_add_error(self, EXPECTED_INTEGER);
        goto end;
    }
    
    String *raw = lexeme(self);
    if(NULL == raw)
    {
        parser_add_lexeme_error(self, location(self));
        goto end;
    }

    errno = 0;
    index = strtoll(C(raw), NULL, 10);
    if(ERANGE == errno && '-' == C(raw)[0])
    {
        parser_add_error(self, INTEGER_TOO_SMALL);
    }
    else if(ERANGE == errno)
    {
        parser_add_error(self, INTEGER_TOO_BIG);
    }

    free(raw);

  end:
    scanner_next(self);
    return index;
}

static void parse_slice_predicate(Parser *self, Predicate *predicate)
{
    predicate->kind = SLICE;

    if(INTEGER_LITERAL == current_kind(self))
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

    if(INTEGER_LITERAL == current_kind(self))
    {
        predicate->slice.specified |= SLICE_TO;
        predicate->slice.to = parse_index(self);
    }

    if(COLON == current_kind(self))
    {
        scanner_next(self);
        if(INTEGER_LITERAL == current_kind(self))
        {
            predicate->slice.specified |= SLICE_STEP;
            predicate->slice.step = parse_index(self);
            if(0 == predicate->slice.step)
            {
                parser_add_error(self, STEP_CANNOT_BE_ZERO);
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
        parser_add_error(self, UNBALANCED_PRED_DELIM);
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
    SourceLocation unquoted = location(self);
    unquoted.location.start.index++;
    unquoted.location.start.offset++;
    unquoted.location.end.index--;
    unquoted.location.end.offset--;

    String *raw = lexeme_at(self, unquoted.location);
    if(NULL == raw)
    {
        parser_add_lexeme_error(self, unquoted.location);
        return;
    }

    if(strlen(raw) < 2)
    {
        // N.B. - nothing to escape here, return early
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

static inline Step *make_step(Parser *self, JsonPath *path, enum step_kind kind)
{
    Step *step = xcalloc(sizeof(Step));
    step->kind = kind;
    step->test.kind = NAME_TEST;
    step->location = location(self);

    vector_add(path->steps, step);

    return step;
}

static void parse_qualified_head_step(Parser *self, JsonPath *path)
{
    scanner_next(self);
    parse_predicate(self, make_step(self, path, ROOT));
}

static void parse_recursive_step(Parser *self, JsonPath *path)
{
    if(DOT_DOT == current_kind(self))
    {
        scanner_next(self);
    }

    parse_step(self, make_step(self, path, RECURSIVE));
}

static void parse_relative_step(Parser *self, JsonPath *path)
{
    if(DOT == current_kind(self))
    {
        scanner_next(self);
    }
    
    parse_step(self, make_step(self, path, SINGLE));
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
