#include <ctype.h>

#include "vector.h"

#include "parser/lexer.h"

static const char * const ERRORS[] =
{
    [PREMATURE_END_OF_INPUT] = "premature end of input",
    [UNSUPPORTED_CONTROL_CHARACTER] = "unsupported control character",
    [UNSUPPORTED_ESCAPE_SEQUENCE] = "unsupported escape sequence"
};

static void add_error(Lexer *self, LexerErrorCode code)
{
    LexerError *err = calloc(1, sizeof(LexerError));
    err->code = code;
    err->position = position(self);
    vector_append(self->errors, err);
}

static void add_error_at(Lexer *self, Position position, LexerErrorCode code)
{
    LexerError *err = calloc(1, sizeof(LexerError));
    err->code = code;
    err->position = position;
    vector_append(self->errors, err);
}

static bool read_hex_sequence(Lexer *self, size_t count)
{
    for(size_t i = 0; i < count; i++)
    {
        if(!input_has_more(&self->input))
        {
            add_error(self, PREMATURE_END_OF_INPUT);
            return false;
        }

        if(!isxdigit(input_peek(&self->input)))
        {
            add_error(self, UNSUPPORTED_ESCAPE_SEQUENCE);
        }
        input_consume_one(&self->input);
    }

    return true;
}

typedef bool (*EscapeSequenceReader)(Lexer *self);

static bool read_escape_sequence(Lexer *self)
{
    if(input_peek(&self->input) == '\\')
    {
        input_consume_one(&self->input);
    }

    if(!input_has_more(&self->input))
    {
        add_error(self, PREMATURE_END_OF_INPUT);
        return false;
    }

    Position start = position(self);
    switch(input_consume_one(&self->input))
    {
        case '"':
        case '\\':
        case '/':
        case ' ':
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
            add_error_at(self, start, UNSUPPORTED_ESCAPE_SEQUENCE);
    }

    return true;
}

static bool read_name_escape_sequence(Lexer *self)
{
    Position start = position(self);
    if(input_peek(&self->input) == '\\')
    {
        input_consume_one(&self->input);
    }
    if(!input_has_more(&self->input))
    {
        add_error(self, PREMATURE_END_OF_INPUT);
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

static void match_quoted_term(Lexer *self, char quote, EscapeSequenceReader reader)
{
    if(input_peek(&self->input) == quote)
    {
        input_consume_one(&self->input);
    }

    bool found_close = false;
    while(!found_close)
    {
        if(!input_has_more(&self->input))
        {
            add_error(self, PREMATURE_END_OF_INPUT);
            break;
        }
        if(iscntrl(input_peek(&self->input)))
        {
            add_error(self, UNSUPPORTED_CONTROL_CHARACTER);
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
            found_close = true;
        }
    }
}

static bool read_digit_sequence(Lexer *self)
{
    Position start = position(self);

    bool done = false;
    while(!done)
    {
        if(!input_has_more(&self->input))
        {
            if(input_index(&self->input) == start.index)
            {
                add_error(self, PREMATURE_END_OF_INPUT);
                break;
            }

            done = true;
            break;
        }

        if(isdigit(input_peek(&self->input)))
        {
            input_consume_one(&self->input);
        }
        else
        {
            done = true;
        }
    }

    return done;
}

static void match_number(Lexer *self)
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

static void match_name(Lexer *self)
{
    Position start = position(self);

    bool done = false;
    while(!done)
    {
        if(!input_has_more(&self->input))
        {
            if(input_index(&self->input) == start.index)
            {
                add_error(self, PREMATURE_END_OF_INPUT);
                return;
            }

            done = true;
            continue;
        }
        if(iscntrl(input_peek(&self->input)))
        {
            add_error(self, UNSUPPORTED_CONTROL_CHARACTER);
        }
    
        char c = input_consume_one(&self->input);
        if(c == '[' || c == '.' || c == '=' || c == ',' || c == '}' || c == ')' || isspace(c))
        {
            input_push_back(&self->input);
            done = true;
        }
    }
}

static void match_symbol(Lexer *self)
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

Lexer *make_lexer(const char *data, size_t length)
{
    Lexer *self = calloc(1, sizeof(Lexer) + length);
    if(NULL == self)
    {
        goto exit;
    }
    self->errors = make_vector();
    if(NULL == self->errors)
    {
        dispose_lexer(self);
        self = NULL;
        goto exit;
    }
    
    if(!input_init(&self->input, NULL, length))
    {
        dispose_lexer(self);
        self = NULL;
        goto exit;
    }

    memcpy(self->input.source.buffer, data, length);

  exit:
    return self;
}

void dispose_lexer(Lexer *self)
{
    if(NULL == self)
    {
        return;
    }

    vector_destroy(self->errors, free);
    input_release(&self->input);
    free(self);
}

void next(Lexer *self)
{
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
            self->current.kind = OPEN_BRACKET;
            break;
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
            self->current.kind = OPEN_PARENTHSIS;
            break;
        case ')':
            self->current.kind = CLOSE_PARENTHESIS;
            break;
        case '?':
        {
            if(input_peek(&self->input) == '(')
            {
                self->current.kind = OPEN_FILTER;
                input_consume_one(&self->input);
                break;
            }
            input_push_back(&self->input);
            match_symbol(self);
            break;
        }
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
            match_quoted_term(self, '"', read_escape_sequence);
            break;
        case '\'':
            self->current.kind = QUOTED_NAME;
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
    Position end = position(self);    
    self->current.location.position = start;
    self->current.location.extent = end.index - start.index;
}

const char *lexer_strerror(LexerErrorCode code)
{
    return ERRORS[code];
}
