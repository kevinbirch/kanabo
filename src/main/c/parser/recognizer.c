#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "parser/context.h"
#include "parser/recognize.h"
#include "parser/escape.h"
#include "xalloc.h"

#define current(PARSER) (PARSER)->scanner->current.kind
#define token(PARSER) (PARSER)->scanner->current
#define next(PARSER) scanner_next((PARSER)->scanner)
#define lexeme(PARSER) scanner_extract_lexeme((PARSER)->scanner, (PARSER)->scanner->current.location)

static inline void expect(Parser *self, TokenKind kind)
{
    bool squaked = false;

    while(kind != current(self))
    {
        if(END_OF_INPUT == current(self))
        {
            add_parser_error(self, position(self), PREMATURE_END_OF_INPUT);
            break;
        }
        if(!squaked)
        {
            squaked = true;
            add_parser_error(self, position(self), UNEXPECTED_INPUT);
        }
        next(self);
    }
}

static inline int64_t parse_index(Parser *self)
{
    Position start = position(self);
    int64_t index = -1;

    bool negate = false;
    if(MINUS == current(self))
    {
        negate = true;
        next(self);

        if(INTEGER_LITERAL != current(self))
        {
            add_parser_error(self, start, EXPECTED_INTEGER);
            goto end;
        }
    }
    
    String *raw = lexeme(self);
    next(self);

    if(NULL == raw)
    {
        Location loc = self->scanner->current.location;
        add_parser_internal_error(self, __FILE__, __LINE__, "can't extract lexeme at %zu:%zu", loc.index, loc.extent);
        goto end;
    }

    errno = 0;
    index = strtoll(C(raw), NULL, 10);
    if(ERANGE == errno && negate)
    {
        add_parser_error(self, start, INTEGER_TOO_SMALL);
    }
    else if(ERANGE == errno)
    {
        add_parser_error(self, start, INTEGER_TOO_BIG);
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

    if(INTEGER_LITERAL == current(self) || MINUS == current(self))
    {
        predicate->slice.specified |= SLICE_FROM;
        predicate->slice.from = parse_index(self);
    }

    if(COLON != current(self))
    {
        add_parser_error(self, position(self), UNEXPECTED_INPUT);
        return;
    }

    // N.B. - eat the colon
    next(self);

    if(INTEGER_LITERAL == current(self) || MINUS == current(self))
    {
        predicate->slice.specified |= SLICE_TO;
        predicate->slice.to = parse_index(self);
    }

    if(COLON == current(self))
    {
        next(self);
        if(INTEGER_LITERAL == current(self) || MINUS == current(self))
        {
            predicate->slice.specified |= SLICE_STEP;
            Position start = position(self);
            predicate->slice.step = parse_index(self);
            if(0 == predicate->slice.step)
            {
                add_parser_error(self, start, STEP_CANNOT_BE_ZERO);
            }
        }
    }
}

static void parse_indexed_predicate(Parser *self, Predicate *predicate)
{
    int64_t index = parse_index(self);

    if(COLON == current(self))
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
    next(self);

    Predicate *predicate = xcalloc(sizeof(Predicate));
    step->predicate = predicate;

    switch(current(self))
    {
        case ASTERISK:
            predicate->kind = WILDCARD;
            next(self);
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
            add_parser_error(self, position(self), EXPECTED_PREDICATE_PRODUCTION);
            next(self);
            break;
        case CLOSE_BRACKET:
            add_parser_error(self, position(self), EXPECTED_PREDICATE_PRODUCTION);
            next(self);
            return;
        case END_OF_INPUT:
            goto unclosed;
            break;
        default:
            add_parser_error(self, position(self), EXPECTED_PREDICATE_PRODUCTION);
            next(self);
            // N.B. - keep going until the predicate closing `]`
            break;
    }

    expect(self, CLOSE_BRACKET);

  unclosed:
    if(CLOSE_BRACKET != current(self))
    {
        add_parser_error(self, start, UNBALANCED_PRED_DELIM);
    }

    next(self);
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
    Location unquoted = 
        {
            .index = loc.index + 1,
            .line = loc.line,
            .offset = loc.offset + 1,
            .extent = loc.extent - 2
        };
        
    String *raw = scanner_extract_lexeme(self->scanner, unquoted);
    if(NULL == raw)
    {
        add_parser_internal_error(self, __FILE__, __LINE__, "can't extract lexeme at %zu:%zu", loc.index, loc.extent);
        return;
    }
    if(strlen(raw) < 2)
    {
        // N.B. - short read on token
        goto cleanup;
    }

    // N.B. - trim leading quote
    String *cooked = unescape(self, raw);
    if(NULL == cooked)
    {
        goto cleanup;
    }

    step->test.name = cooked;

  cleanup:
    string_free(raw);
}

static void recover(Parser *self, Step *step)
{
    next(self);

    while(true)
    {
        switch(current(self))
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
                next(self);
                break;
        }
    }
}

static void parse_step(Parser *self, Step *step)
{
    switch(current(self))
    {
        case ASTERISK:
            step->test.kind = WILDCARD_TEST;
            break;
        case QUOTED_NAME:
            parse_quoted_name(self, step);
            break;
        case NAME:
            step->test.name = lexeme(self);
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
            add_parser_error(self, position(self), EXPECTED_STEP_PRODUCTION);
            return;
        default:
            add_parser_error(self, position(self), EXPECTED_STEP_PRODUCTION);
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

JsonPath *recognize(Parser *self)
{
    JsonPath *path = make_jsonpath(RELATIVE_PATH);

    next(self);
    parse_head_step(self, path);

    bool done = false;
    while(!done)
    {
        switch(current(self))
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
                add_parser_error(self, position(self), EXPECTED_QUALIFIED_STEP_PRODUCTION);
                done = true;
                break;
        }
    }

    return path;
}
