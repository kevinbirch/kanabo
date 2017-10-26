#include <ctype.h>

#include "parser/lexer.h"

#define const_token(KIND, EXTENT) (Token){.kind=(KIND), .lexeme.location.extent=(EXTENT)}
#define dyn_token(KIND) (Token){.kind=(KIND)}

static const Token PROTOTYPES[] =
{
    [END_OF_INPUT] = const_token(END_OF_INPUT, 0),
    [DOLLAR] = const_token(DOLLAR, 1),
    [AT] = const_token(AT, 1),
    [DOT_DOT] = const_token(DOT_DOT, 2),
    [DOT] = const_token(DOT, 1),
    [EQUALS] = const_token(EQUALS, 1),
    [COLON] = const_token(COLON, 1),
    [COMMA] = const_token(COMMA, 1),
    [EXCLAMATION] = const_token(EXCLAMATION, 1),
    [AMPERSAND] = const_token(AMPERSAND, 1),
    [ASTERISK] = const_token(ASTERISK, 1),
    [OPEN_BRACKET] = const_token(OPEN_BRACKET, 1),
    [CLOSE_BRACKET] = const_token(CLOSE_BRACKET, 1),
    [OPEN_BRACE] = const_token(OPEN_BRACE, 1),
    [CLOSE_BRACE] = const_token(CLOSE_BRACE, 1),
    [OPEN_PARENTHSIS] = const_token(OPEN_PARENTHSIS, 1),
    [CLOSE_PARENTHESIS] = const_token(CLOSE_PARENTHESIS, 1),
    [OPEN_FILTER] = const_token(OPEN_FILTER, 2),
    [GREATER_THAN] = const_token(GREATER_THAN, 1),
    [GREATER_THAN_EQUAL] = const_token(GREATER_THAN_EQUAL, 2),
    [LESS_THAN] = const_token(LESS_THAN, 1),
    [LESS_THAN_EQUAL] = const_token(LESS_THAN_EQUAL, 2),
    [NOT_EQUAL] = const_token(NOT_EQUAL, 2),
    [PLUS] = const_token(PLUS, 1),
    [MINUS] = const_token(MINUS, 1),
    [SLASH] = const_token(SLASH, 1),
    [PERCENT] = const_token(PERCENT, 1),
    [OBJECT_SELECTOR] = const_token(OBJECT_SELECTOR, 8),
    [ARRAY_SELECTOR] = const_token(ARRAY_SELECTOR, 7),
    [STRING_SELECTOR] = const_token(STRING_SELECTOR, 8),
    [NUMBER_SELECTOR] = const_token(NUMBER_SELECTOR, 8),
    [INTEGER_SELECTOR] = const_token(INTEGER_SELECTOR, 9),
    [DECIMAL_SELECTOR] = const_token(DECIMAL_SELECTOR, 9),
    [TIMESTAMP_SELECTOR] = const_token(TIMESTAMP_SELECTOR, 11),
    [BOOLEAN_SELECTOR] = const_token(BOOLEAN_SELECTOR, 9),
    [NULL_SELECTOR] = const_token(NULL_SELECTOR, 6),
    [NULL_LITERAL] = const_token(NULL_LITERAL, 4),
    [BOOLEAN_OR] = const_token(BOOLEAN_OR, 2),
    [BOOLEAN_AND] = const_token(BOOLEAN_AND, 3),
    [BOOLEAN_LITERAL_TRUE] = const_token(BOOLEAN_LITERAL_TRUE, 4),
    [BOOLEAN_LITERAL_FALSE] = const_token(BOOLEAN_LITERAL_FALSE, 5),
    [STRING_LITERAL] = dyn_token(STRING_LITERAL),
    [INTEGER_LITERAL] = dyn_token(INTEGER_LITERAL),
    [REAL_LITERAL] = dyn_token(REAL_LITERAL),
    [QUOTED_NAME] = dyn_token(QUOTED_NAME),
    [NAME] = dyn_token(NAME),
};

make_maybe(size_t);

static const char * const ERRORS[] =
{
    [PREMATURE_END_OF_INPUT] = "premature end of input",
    [UNSUPPORTED_CONTROL_CHARACTER] = "unsupported control character",
    [UNSUPPORTED_ESCAPE_SEQUENCE] = "unsupported escape sequence"
};

static Maybe(size_t) read_hex_sequence(Input *input, size_t count)
{
    if(count > input_remaining(input))
    {
        return fail(size_t, PREMATURE_END_OF_INPUT);
    }

    for(size_t i = 0; i < count; i++)
    {
        if(!isxdigit(input_peek(input)))
        {
            return fail(size_t, UNSUPPORTED_ESCAPE_SEQUENCE);

        }
        input_consume_one(input);
    }

    return just(size_t, count);
}

#define abort_if(OPTION) if(is_nothing((OPTION))) return (OPTION)

typedef Maybe(size_t) (*EscapeSequenceReader)(Input *input);

static Maybe(size_t) read_escape_sequence(Input *input)
{
    size_t length = 0;
    if(input_peek(input) == '\\')
    {
        input_consume_one(input);
        length++;
    }
    if(!input_has_more(input))
    {
        return fail(size_t, PREMATURE_END_OF_INPUT);
    }

    char c = input_consume_one(input);
    length++;
    switch(c)
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
        {
            Maybe(size_t) seq = read_hex_sequence(input, 2);
            length += maybe(seq, 0);
            abort_if(seq);
            break;
        }
        case 'u':
        {
            Maybe(size_t) seq = read_hex_sequence(input, 4);
            length += maybe(seq, 0);
            abort_if(seq);
            break;
        }
            break;
        case 'U':
        {
            Maybe(size_t) seq = read_hex_sequence(input, 8);
            length += maybe(seq, 0);
            abort_if(seq);
            break;
        }
            break;
        default:
            return fail(size_t, UNSUPPORTED_ESCAPE_SEQUENCE);
    }

    return just(size_t, length);
}

static Maybe(size_t) read_name_escape_sequence(Input *input)
{
    input_push_mark(input);
    size_t length = 0;
    if(input_peek(input) == '\\')
    {
        length++;
        input_consume_one(input);
    }
    if(!input_has_more(input))
    {
        input_drop_mark(input);
        return fail(size_t, PREMATURE_END_OF_INPUT);
    }

    if(input_peek(input) == '\'')
    {
        input_drop_mark(input);
        length++;
        input_consume_one(input);
        return just(size_t, length);
    }

    input_pop_mark(input);
    return read_escape_sequence(input);
}

static Maybe(Token) match_quoted_term(Input *input, char quote, enum token_kind kind, EscapeSequenceReader reader)
{
    if(input_peek(input) == quote)
    {
        input_consume_one(input);
    }

    Position start = input_position(input);

    bool found_close = false;
    while(!found_close)
    {
        if(!input_has_more(input))
        {
            return fail(Token, PREMATURE_END_OF_INPUT);
        }
        if(iscntrl(input_peek(input)))
        {
            return fail(Token, UNSUPPORTED_CONTROL_CHARACTER);
        }
        if(input_peek(input) == '\\')
        {
            Maybe(size_t) seq = reader(input);
            if(is_nothing(seq))
            {
                return fail(Token, from_nothing(seq));
            }
        }
    
        char c = input_consume_one(input);
        if(c == quote)
        {
            found_close = true;
        }
    }

    Position end = input_position(input);    
    Token name = PROTOTYPES[kind];
    name.lexeme = (SourceLocation){.input=input, .location.position=start};
    name.lexeme.location.extent = end.index - start.index - 1;
    
    return just(Token, name);
}


static Maybe(size_t) read_digit_sequence(Input *input)
{
    Position start = input_position(input);

    bool done = false;
    while(!done)
    {
        if(!input_has_more(input))
        {
            if(input_index(input) == start.index)
            {
                return fail(size_t, PREMATURE_END_OF_INPUT);
            }

            done = true;
            continue;
        }

        if(isdigit(input_peek(input)))
        {
            input_consume_one(input);
        }
        else
        {
            done = true;
        }
    }

    Position end = input_position(input);    
    return just(size_t, end.index - start.index);
}

static Maybe(Token) match_number(Input *input)
{
    Position start = input_position(input);

    enum token_kind found = INTEGER_LITERAL;

    Maybe(size_t) seq = read_digit_sequence(input);
    if(is_nothing(seq))
    {
        return fail(Token, from_nothing(seq));
    }

    if(input_peek(input) == '.')
    {
        found = REAL_LITERAL;
        input_consume_one(input);
        seq = read_digit_sequence(input);
        if(is_nothing(seq))
        {
            return fail(Token, from_nothing(seq));
        }
    }

    if(input_peek(input) == 'e')
    {
        input_consume_one(input);
        seq = read_digit_sequence(input);
        if(is_nothing(seq))
        {
            return fail(Token, from_nothing(seq));
        }
    }

    Position end = input_position(input);    
    Token name = PROTOTYPES[found];
    name.lexeme = (SourceLocation){.input=input, .location.position=start};
    name.lexeme.location.extent = end.index - start.index;
    
    return just(Token, name);
}

static Maybe(Token) match_name(Input *input)
{
    Position start = input_position(input);

    bool done = false;
    while(!done)
    {
        if(!input_has_more(input))
        {
            if(input_index(input) == start.index)
            {
                return fail(Token, PREMATURE_END_OF_INPUT);
            }

            done = true;
            continue;
        }
        if(iscntrl(input_peek(input)))
        {
            return fail(Token, UNSUPPORTED_CONTROL_CHARACTER);
        }
    
        char c = input_consume_one(input);
        if(c == '.' || c == '[')
        {
            input_push_back(input);
            done = true;
        }
    }

    Position end = input_position(input);    
    Token name = PROTOTYPES[NAME];
    name.lexeme = (SourceLocation){.input=input, .location.position=start};
    name.lexeme.location.extent = end.index - start.index;
    
    return just(Token, name);
}

static Maybe(Token) match_symbol(Input *input)
{
    Position start = input_position(input);
    enum token_kind found = END_OF_INPUT;
    if(input_consume_if(input, "object()"))
    {
        found = OBJECT_SELECTOR;
    }
    else if(input_consume_if(input, "array()"))
    {
        found = ARRAY_SELECTOR;
    }
    else if(input_consume_if(input, "string()"))
    {
        found = STRING_SELECTOR;
    }
    else if(input_consume_if(input, "number()"))
    {
        found = NUMBER_SELECTOR;
    }
    else if(input_consume_if(input, "integer()"))
    {
        found = INTEGER_SELECTOR;
    }
    else if(input_consume_if(input, "decimal()"))
    {
        found = DECIMAL_SELECTOR;
    }
    else if(input_consume_if(input, "timestamp()"))
    {
        found = TIMESTAMP_SELECTOR;
    }
    else if(input_consume_if(input, "boolean()"))
    {
        found = BOOLEAN_SELECTOR;
    }
    else if(input_consume_if(input, "null"))
    {
        found = NULL_LITERAL;
        if(input_consume_if(input, "()"))
        {
            found = NULL_SELECTOR;
        }
    }
    else if(input_consume_if(input, "and"))
    {
        found = BOOLEAN_SELECTOR;
    }
    else if(input_consume_if(input, "or"))
    {
        found = BOOLEAN_SELECTOR;
    }
    else if(input_consume_if(input, "true"))
    {
        found = BOOLEAN_SELECTOR;
    }
    else if(input_consume_if(input, "false"))
    {
        found = BOOLEAN_SELECTOR;
    }
    else
    {
        return match_name(input);
    }

    Token proto = PROTOTYPES[found];
    proto.lexeme.input = input;
    proto.lexeme.location.position = start;
    return just(Token, proto);
}

static Maybe(Token) match_term(Input *input)
{
    char current = input_peek(input);
    if(current == '-')
    {
        input_push_mark(input);
        Maybe(Token) number = match_number(input);
        if(is_just(number))
        {
            return number;
        }

        input_reset_to_mark(input);
        return match_name(input);
    }
    else if(isdigit(current))
    {
        return match_number(input);
    }

    return match_symbol(input);
}

Maybe(Token) next(Input *input) 
{
    input_skip_whitespace(input);

    enum token_kind found = END_OF_INPUT;

    Position start = input_position(input);
    if(!input_has_more(input))
    {
        goto finish;
    }

    char c = input_consume_one(input);
    switch(c)
    {
        case '$':
            found = DOLLAR;
            break;
        case '@':
            found = AT;
            break;
        case '.':
        {
            found = DOT;
            if(input_peek(input) == '.')
            {
                found = DOT_DOT;
                input_consume_one(input);
            }
            break;
        }
        case '=':
            found = EQUALS;
            break;
        case ':':
            found = COLON;
            break;
        case ',':
            found = COMMA;
            break;
        case '!':
        {
            found = EXCLAMATION;
            if(input_peek(input) == '=')
            {
                found = NOT_EQUAL;
                input_consume_one(input);
            }
            break;
        }
        case '&':
            found = AMPERSAND;
            break;
        case '*':
            found = ASTERISK;
            break;
        case '[':
            found = OPEN_BRACKET;
            break;
        case ']':
            found = CLOSE_BRACKET;
            break;
        case '{':
            found = OPEN_BRACE;
            break;
        case '}':
            found = CLOSE_BRACE;
            break;
        case '(':
            found = OPEN_PARENTHSIS;
            break;
        case ')':
            found = CLOSE_PARENTHESIS;
            break;
        case '?':
        {
            if(input_peek(input) == '(')
            {
                found = OPEN_FILTER;
                input_consume_one(input);
            }
            break;
        }
        case '<':
        {
            found = LESS_THAN;
            if(input_peek(input) == '=')
            {
                found = LESS_THAN_EQUAL;
                input_consume_one(input);
            }
            break;
        }
        case '>':
        {
            found = GREATER_THAN;
            if(input_peek(input) == '=')
            {
                found = GREATER_THAN_EQUAL;
                input_consume_one(input);
            }
            break;
        }
        case '+':
            found = PLUS;
            break;
        case '-':
        {
            found = MINUS;
            break;
        }
        case '/':
            found = SLASH;
            break;
        case '%':
            found = PERCENT;
            break;
        case '"':
            return match_quoted_term(input, '"', STRING_LITERAL, read_escape_sequence);
        case '\'':
            return match_quoted_term(input, '\'', QUOTED_NAME, read_name_escape_sequence);
        default:
        {
            input_push_back(input);
            return match_term(input);
        }
    }

  finish:
    Token proto = PROTOTYPES[found];
    proto.lexeme.input = input;
    proto.lexeme.location.position = start;
    return just(Token, proto);
}

const char *lexer_strerror(LexerFailureCode code)
{
    return ERRORS[code];
}
