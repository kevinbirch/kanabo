#include <ctype.h>

#include "conditions.h"
#include "vector.h"
#include "xalloc.h"

#include "parser/scanner.h"

#define scan_position(PARSER) (PARSER)->input->position

static bool read_hex_sequence(Parser *self, size_t count)
{
    bool squelch = false;

    for(size_t i = 0; i < count; i++)
    {
        if(!input_has_more(self->input))
        {
            scanner_add_error(self, PREMATURE_END_OF_INPUT);
            return false;
        }

        size_t point = self->input->position.index;

        if(!isxdigit(input_consume_one(self->input)) && !squelch)
        {
            squelch = true;
            scanner_add_error_at(self, UNSUPPORTED_ESCAPE_SEQUENCE, location(self), point);
        }
    }

    return true;
}

typedef bool (*EscapeSequenceReader)(Parser *self);

static bool read_escape_sequence(Parser *self)
{
    if(input_peek(self->input) == '\\')
    {
        input_consume_one(self->input);
    }

    if(!input_has_more(self->input))
    {
        scanner_add_error(self, PREMATURE_END_OF_INPUT);
        return false;
    }

    size_t point = self->input->position.index;

    switch(input_consume_one(self->input))
    {
        case '"':
        case '\\':
        case '/':
        case ' ':
        case '_':
        case '0':
        case 'a':
        case 'b':
        case 'e':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case 'L':
        case 'N':
        case 'P':
            break;
        case 'x':
            return read_hex_sequence(self, 2);
        case 'u':
            return read_hex_sequence(self, 4);
        case 'U':
            return read_hex_sequence(self, 8);
        default:
            scanner_add_error_at(self, UNSUPPORTED_ESCAPE_SEQUENCE, location(self), point);
    }

    return true;
}

static bool read_name_escape_sequence(Parser *self)
{
    Position start = scan_position(self);

    if(input_peek(self->input) == '\\')
    {
        input_consume_one(self->input);
    }

    if(!input_has_more(self->input))
    {
        scanner_add_error(self, PREMATURE_END_OF_INPUT);
        return false;
    }

    if(input_peek(self->input) == '\'')
    {
        input_consume_one(self->input);
        return true;
    }

    input_goto(self->input, start);
    return read_escape_sequence(self);
}

static void match_quoted_term(Parser *self, char quote, EscapeSequenceReader reader)
{
    size_t point = self->input->position.index;

    if(input_peek(self->input) == quote)
    {
        input_consume_one(self->input);
    }

    bool done = false;

    while(!done)
    {
        if(!input_has_more(self->input))
        {
            scanner_add_error_at(self, UNCLOSED_QUOTATION, location(self), point);
            scanner_add_error(self, PREMATURE_END_OF_INPUT);
            break;
        }

        char c = input_peek(self->input);

        if(c == '\\')
        {
            if(reader(self))
            {
                continue;
            }

            break;
        }
        else if(iscntrl(c))
        {
            scanner_add_error(self, UNSUPPORTED_CONTROL_CHARACTER);
        }
        else if(c == quote)
        {
            done = true;
        }

        input_consume_one(self->input);
    }
}

static bool read_digit_sequence(Parser *self)
{
    if(!input_has_more(self->input))
    {
        scanner_add_error(self, PREMATURE_END_OF_INPUT);
        return false;
    }

    while(true)
    {
        if(!input_has_more(self->input) || !isdigit(input_peek(self->input)))
        {
            break;
        }

        input_consume_one(self->input);
    }

    return true;
}

static void match_number(Parser *self)
{
    self->current.kind = INTEGER_LITERAL;

    if(isdigit(input_peek(self->input)))
    {
        if(!read_digit_sequence(self))
        {
            return;
        }
    }

    if(input_peek(self->input) == '.')
    {
        self->current.kind = REAL_LITERAL;
        input_consume_one(self->input);
        if(!read_digit_sequence(self))
        {
            return;
        }
    }

    if(input_peek(self->input) == 'e')
    {
        input_consume_one(self->input);
        if(!read_digit_sequence(self))
        {
            return;
        }
    }
}

static void match_name(Parser *self)
{
    if(!input_has_more(self->input))
    {
        scanner_add_error(self, PREMATURE_END_OF_INPUT);
        return;
    }

    while(true)
    {
        if(!input_has_more(self->input))
        {
            return;
        }

        char c = input_peek(self->input);

        if(iscntrl(c))
        {
            scanner_add_error(self, UNSUPPORTED_CONTROL_CHARACTER);
            goto consume;
        }
        else if(isspace(c))
        {
            return;
        }
        switch(c)
        {
            case '[':
            case ']':
            case '.':
            case '=':
            case ',':
            case '}':
            case '(':
            case ')':
                return;
            default:
                break;
        }

      consume:
        input_consume_one(self->input);
    }
}

static void match_symbol(Parser *self)
{
    if(!input_has_more(self->input))
    {
        scanner_add_error(self, PREMATURE_END_OF_INPUT);
        return;
    }

    if(input_consume_if(self->input, "object()"))
    {
        self->current.kind = OBJECT_SELECTOR;
    }
    else if(input_consume_if(self->input, "array()"))
    {
        self->current.kind = ARRAY_SELECTOR;
    }
    else if(input_consume_if(self->input, "string()"))
    {
        self->current.kind = STRING_SELECTOR;
    }
    else if(input_consume_if(self->input, "number()"))
    {
        self->current.kind = NUMBER_SELECTOR;
    }
    else if(input_consume_if(self->input, "integer()"))
    {
        self->current.kind = INTEGER_SELECTOR;
    }
    else if(input_consume_if(self->input, "decimal()"))
    {
        self->current.kind = DECIMAL_SELECTOR;
    }
    else if(input_consume_if(self->input, "timestamp()"))
    {
        self->current.kind = TIMESTAMP_SELECTOR;
    }
    else if(input_consume_if(self->input, "boolean()"))
    {
        self->current.kind = BOOLEAN_SELECTOR;
    }
    else if(input_consume_if(self->input, "null"))
    {
        self->current.kind = NULL_LITERAL;
        if(input_consume_if(self->input, "()"))
        {
            self->current.kind = NULL_SELECTOR;
        }
    }
    else if(input_consume_if(self->input, "and"))
    {
        self->current.kind = BOOLEAN_AND;
    }
    else if(input_consume_if(self->input, "or"))
    {
        self->current.kind = BOOLEAN_OR;
    }
    else if(input_consume_if(self->input, "true"))
    {
        self->current.kind = BOOLEAN_LITERAL_TRUE;
    }
    else if(input_consume_if(self->input, "false"))
    {
        self->current.kind = BOOLEAN_LITERAL_FALSE;
    }
    else
    {
        self->current.kind = NAME;
        match_name(self);
    }
}

void scanner_next(Parser *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    if(END_OF_INPUT == self->current.kind)
    {
        return;
    }

    size_t error_count = vector_length(self->errors);

    input_skip_whitespace(self->input);

    self->current.location.start = scan_position(self);

    if(!input_has_more(self->input))
    {
        self->current.kind = END_OF_INPUT;
        goto finish;
    }

    switch(input_consume_one(self->input))
    {
        case '$':
            self->current.kind = DOLLAR;
            break;
        case '@':
            self->current.kind = AT;
            break;
        case '.':
        {
            self->current.kind = DOT;
            char next = input_peek(self->input);
            if(next == '.')
            {
                self->current.kind = DOT_DOT;
                input_consume_one(self->input);
            }
            else if(isdigit(next))
            {
                input_push_back(self->input);
                match_number(self);
            }
            break;
        }
        case '=':
            self->current.kind = EQUALS;
            break;
        case ':':
            self->current.kind = COLON;
            break;
        case ',':
            self->current.kind = COMMA;
            break;
        case '!':
        {
            self->current.kind = EXCLAMATION;
            if(input_peek(self->input) == '=')
            {
                self->current.kind = NOT_EQUAL;
                input_consume_one(self->input);
            }
            break;
        }
        case '&':
            self->current.kind = AMPERSAND;
            break;
        case '*':
            self->current.kind = ASTERISK;
            break;
        case '[':
        {
            self->current.kind = OPEN_BRACKET;
            if(input_peek(self->input) == '?')
            {
                self->current.kind = OPEN_FILTER;
                input_consume_one(self->input);
            }
            break;
        }
        case ']':
            self->current.kind = CLOSE_BRACKET;
            break;
        case '{':
            self->current.kind = OPEN_BRACE;
            break;
        case '}':
            self->current.kind = CLOSE_BRACE;
            break;
        case '(':
            self->current.kind = OPEN_PARENTHESIS;
            break;
        case ')':
            self->current.kind = CLOSE_PARENTHESIS;
            break;
        case '<':
        {
            self->current.kind = LESS_THAN;
            if(input_peek(self->input) == '=')
            {
                self->current.kind = LESS_THAN_EQUAL;
                input_consume_one(self->input);
            }
            break;
        }
        case '>':
        {
            self->current.kind = GREATER_THAN;
            if(input_peek(self->input) == '=')
            {
                self->current.kind = GREATER_THAN_EQUAL;
                input_consume_one(self->input);
            }
            break;
        }
        case '+':
            self->current.kind = PLUS;
            if(isdigit(input_peek(self->input)))
            {
                match_number(self);
            }
            break;
        case '-':
            self->current.kind = MINUS;
            if(isdigit(input_peek(self->input)))
            {
                match_number(self);
            }
            break;
        case '/':
            self->current.kind = SLASH;
            break;
        case '%':
            self->current.kind = PERCENT;
            break;
        case '"':
            self->current.kind = STRING_LITERAL;
            input_push_back(self->input);
            match_quoted_term(self, '"', read_escape_sequence);
            break;
        case '\'':
            self->current.kind = QUOTED_NAME;
            input_push_back(self->input);
            match_quoted_term(self, '\'', read_name_escape_sequence);
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            input_push_back(self->input);
            match_number(self);
            break;
        default:
            input_push_back(self->input);
            match_symbol(self);
    }

  finish:
    ;
    self->current.location.end = scan_position(self);
    size_t new_error_count = vector_length(self->errors);
    if(error_count < new_error_count)
    {
        // N.B. - fixup the end positions for new errors
        // xxx - this feels clumsy, is there a better way?
        for(size_t i = error_count; i < new_error_count; i++)
        {
            ParserError *err = vector_get(self->errors, i);
            if(INTERNAL_ERROR == err->code)
            {
                continue;
            }
            err->location.end = self->current.location.end;
        }
    }
}

void scanner_reset(Parser *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    input_reset(self->input);
}

String *scanner_extract_lexeme(Parser *self, Location location)
{
    ENSURE_NONNULL_ELSE_NULL(self);

    return input_extract(self->input, location);
}
