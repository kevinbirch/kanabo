#include <ctype.h>

#include "conditions.h"
#include "parser/scanner.h"
#include "vector.h"
#include "xalloc.h"

#define position(SCANNER) (SCANNER)->input.position

static inline void add_scanner_error_at(Scanner *self, Position position, ParserErrorCode code)
{
    if(NULL == self->handler.callback)
    {
        return;
    }

    self->handler.callback(position, code, self->handler.parameter);
}

static inline void add_scanner_error(Scanner *self, ParserErrorCode code)
{
    add_scanner_error_at(self, position(self), code);
}

static bool read_hex_sequence(Scanner *self, size_t count)
{
    for(size_t i = 0; i < count; i++)
    {
        if(!input_has_more(&self->input))
        {
            add_scanner_error(self, PREMATURE_END_OF_INPUT);
            return false;
        }

        if(!isxdigit(input_peek(&self->input)))
        {
            add_scanner_error(self, UNSUPPORTED_ESCAPE_SEQUENCE);
        }
        input_consume_one(&self->input);
    }

    return true;
}

typedef bool (*EscapeSequenceReader)(Scanner *self);

static bool read_escape_sequence(Scanner *self)
{
    if(input_peek(&self->input) == '\\')
    {
        input_consume_one(&self->input);
    }

    if(!input_has_more(&self->input))
    {
        add_scanner_error(self, PREMATURE_END_OF_INPUT);
        return false;
    }

    Position start = position(self);
    switch(input_consume_one(&self->input))
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
            add_scanner_error_at(self, start, UNSUPPORTED_ESCAPE_SEQUENCE);
    }

    return true;
}

static bool read_name_escape_sequence(Scanner *self)
{
    Position start = position(self);
    if(input_peek(&self->input) == '\\')
    {
        input_consume_one(&self->input);
    }
    if(!input_has_more(&self->input))
    {
        add_scanner_error(self, PREMATURE_END_OF_INPUT);
        return false;
    }

    if(input_peek(&self->input) == '\'')
    {
        input_consume_one(&self->input);
        return true;
    }

    input_goto(&self->input, start);
    return read_escape_sequence(self);
}

static void match_quoted_term(Scanner *self, char quote, EscapeSequenceReader reader)
{
    Position start = position(self);

    if(input_peek(&self->input) == quote)
    {
        input_consume_one(&self->input);
    }

    while(true)
    {
        if(!input_has_more(&self->input))
        {
            add_scanner_error_at(self, start, UNCLOSED_QUOTATION);
            add_scanner_error(self, PREMATURE_END_OF_INPUT);
            break;
        }
        if(iscntrl(input_peek(&self->input)))
        {
            add_scanner_error(self, UNSUPPORTED_CONTROL_CHARACTER);
        }
        if(input_peek(&self->input) == '\\')
        {
            if(reader(self))
            {
                continue;
            }

            break;
        }
    
        char c = input_consume_one(&self->input);
        if(c == quote)
        {
            break;
        }
    }
}

static bool read_digit_sequence(Scanner *self)
{
    Position start = position(self);

    while(true)
    {
        if(!input_has_more(&self->input))
        {
            if(input_index(&self->input) == start.index)
            {
                add_scanner_error(self, PREMATURE_END_OF_INPUT);
                return false;
            }

            break;
        }

        if(isdigit(input_peek(&self->input)))
        {
            input_consume_one(&self->input);
        }
        else
        {
            break;
        }
    }

    return true;
}

static void match_number(Scanner *self)
{
    self->current.kind = INTEGER_LITERAL;

    char next = input_peek(&self->input);
    if('0' <= next && next <= '9')
    {
        if(!read_digit_sequence(self))
        {
            return;
        }
    }

    if(input_peek(&self->input) == '.')
    {
        self->current.kind = REAL_LITERAL;
        input_consume_one(&self->input);
        if(!read_digit_sequence(self))
        {
            return;
        }
    }

    if(input_peek(&self->input) == 'e')
    {
        input_consume_one(&self->input);
        if(!read_digit_sequence(self))
        {
            return;
        }
    }
}

static void match_name(Scanner *self)
{
    Position start = position(self);

    while(true)
    {
        if(!input_has_more(&self->input))
        {
            if(input_index(&self->input) == start.index)
            {
                add_scanner_error(self, PREMATURE_END_OF_INPUT);
            }

            break;
        }

        if(iscntrl(input_peek(&self->input)))
        {
            add_scanner_error(self, UNSUPPORTED_CONTROL_CHARACTER);
            break;
        }

        char c = input_peek(&self->input);
        if(isspace(c))
        {
            break;
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
                input_consume_one(&self->input);
        }
    }
}

static void match_symbol(Scanner *self)
{
    if(input_consume_if(&self->input, "object()"))
    {
        self->current.kind = OBJECT_SELECTOR;
    }
    else if(input_consume_if(&self->input, "array()"))
    {
        self->current.kind = ARRAY_SELECTOR;
    }
    else if(input_consume_if(&self->input, "string()"))
    {
        self->current.kind = STRING_SELECTOR;
    }
    else if(input_consume_if(&self->input, "number()"))
    {
        self->current.kind = NUMBER_SELECTOR;
    }
    else if(input_consume_if(&self->input, "integer()"))
    {
        self->current.kind = INTEGER_SELECTOR;
    }
    else if(input_consume_if(&self->input, "decimal()"))
    {
        self->current.kind = DECIMAL_SELECTOR;
    }
    else if(input_consume_if(&self->input, "timestamp()"))
    {
        self->current.kind = TIMESTAMP_SELECTOR;
    }
    else if(input_consume_if(&self->input, "boolean()"))
    {
        self->current.kind = BOOLEAN_SELECTOR;
    }
    else if(input_consume_if(&self->input, "null"))
    {
        self->current.kind = NULL_LITERAL;
        if(input_consume_if(&self->input, "()"))
        {
            self->current.kind = NULL_SELECTOR;
        }
    }
    else if(input_consume_if(&self->input, "and"))
    {
        self->current.kind = BOOLEAN_AND;
    }
    else if(input_consume_if(&self->input, "or"))
    {
        self->current.kind = BOOLEAN_OR;
    }
    else if(input_consume_if(&self->input, "true"))
    {
        self->current.kind = BOOLEAN_LITERAL_TRUE;
    }
    else if(input_consume_if(&self->input, "false"))
    {
        self->current.kind = BOOLEAN_LITERAL_FALSE;
    }
    else
    {
        self->current.kind = NAME;
        match_name(self);
    }
}

Scanner *make_scanner(const char *data, size_t length)
{
    ENSURE_NONNULL_ELSE_NULL(data);
    ENSURE_ELSE_NULL(0 != length);

    Scanner *self = xcalloc(sizeof(Scanner) + length);
    input_init(&self->input, NULL, length);
    self->input.track_lines = true;
    memcpy(self->input.source.buffer, data, length);

    return self;
}

void dispose_scanner(Scanner *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    input_release(&self->input);
    free(self);
}

void scanner_next(Scanner *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    if(END_OF_INPUT == self->current.kind)
    {
        return;
    }

    input_skip_whitespace(&self->input);

    Position start = position(self);
    if(!input_has_more(&self->input))
    {
        self->current.kind = END_OF_INPUT;
        goto finish;
    }

    switch(input_consume_one(&self->input))
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
            char next = input_peek(&self->input);
            if(next == '.')
            {
                self->current.kind = DOT_DOT;
                input_consume_one(&self->input);
            }
            else if('0' <= next && next <= '9')
            {
                input_push_back(&self->input);
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
            if(input_peek(&self->input) == '=')
            {
                self->current.kind = NOT_EQUAL;
                input_consume_one(&self->input);
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
            if(input_peek(&self->input) == '?')
            {
                self->current.kind = OPEN_FILTER;
                input_consume_one(&self->input);
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
            if(input_peek(&self->input) == '=')
            {
                self->current.kind = LESS_THAN_EQUAL;
                input_consume_one(&self->input);
            }
            break;
        }
        case '>':
        {
            self->current.kind = GREATER_THAN;
            if(input_peek(&self->input) == '=')
            {
                self->current.kind = GREATER_THAN_EQUAL;
                input_consume_one(&self->input);
            }
            break;
        }
        case '+':
            self->current.kind = PLUS;
            break;
        case '-':
            self->current.kind = MINUS;
            break;
        case '/':
            self->current.kind = SLASH;
            break;
        case '%':
            self->current.kind = PERCENT;
            break;
        case '"':
            self->current.kind = STRING_LITERAL;
            input_push_back(&self->input);
            match_quoted_term(self, '"', read_escape_sequence);
            break;
        case '\'':
            self->current.kind = QUOTED_NAME;
            input_push_back(&self->input);
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
            input_push_back(&self->input);
            match_number(self);
            break;
        default:
            input_push_back(&self->input);
            match_symbol(self);
    }

  finish:
    ;
    Position end = position(self);    
    self->current.location.position = start;
    self->current.location.extent = end.index - start.index;
}

void scanner_reset(Scanner *self)
{
    ENSURE_NONNULL_ELSE_VOID(self);

    input_reset(&self->input);
}

String *scanner_extract_lexeme(Scanner *self, Location location)
{
    ENSURE_NONNULL_ELSE_NULL(self);

    return input_extract(&self->input, location);
}
