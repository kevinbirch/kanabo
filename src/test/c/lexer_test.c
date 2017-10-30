#include <stdio.h>

#include "test.h"

// check defines a fail helper that conflicts with the maybe constructor
#undef fail

#include "parser/lexer.h"

#define expected_token(KIND, EXTENT, INDEX) (Token){.kind=(KIND), .location.extent=(EXTENT), .location.index=(INDEX), .location.line=0, .location.offset=0}

#define assert_token(E, A) ck_assert_msg(E.kind == A.kind, "Assertion '"#E".kind == "#A".kind' failed: "#E".kind==%s, "#A".kind==%s", token_name(E.kind), token_name(A.kind)); \
    assert_uint_eq(E.location.index, A.location.index);                 \
    assert_uint_eq(E.location.extent, A.location.extent);               \
    assert_uint_eq(E.location.line, A.location.line);                   \
    assert_uint_eq(E.location.offset, A.location.offset)
#define assert_expectations(L, E) assert_true(vector_is_empty(L->errors)); \
    for(size_t i = 0; i < sizeof(E)/sizeof(Token); i++)                 \
    {                                                                   \
        next(L);                                                        \
        Token expected = E[i];                                          \
        assert_token(expected, L->current);                             \
    }
#define assert_errors(L, X, E) assert_false(!vector_is_empty(L->errors));\
    for(size_t i = 0; i < sizeof(X)/sizeof(Token); i++)                 \
    {                                                                   \
        next(L);                                                        \
        assert_token(X[i], L->current);                                 \
    }                                                                   \
    assert_uint_eq(sizeof(E)/sizeof(LexerError), vector_length(L->errors)); \
    for(size_t i = 0; i < sizeof(E)/sizeof(LexerError); i++)            \
    {                                                                   \
        LexerError *err = vector_get(L->errors, i);                     \
        assert_not_null(err);                                           \
        assert_uint_eq(E[i].code, err->code);                           \
        assert_uint_eq(E[i].position.index, err->position.index);       \
    }

START_TEST (basic)
{
    char *expression = "$.foo.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(NAME, 3, 6),
        expected_token(END_OF_INPUT, 0, 9),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (dotdot)
{
    char *expression = "$.foo..bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT_DOT, 2, 5),
        expected_token(NAME, 3, 7),
        expected_token(END_OF_INPUT, 0, 10),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (wildcard)
{
    char *expression = "$.foo.*";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(ASTERISK, 1, 6),
        expected_token(END_OF_INPUT, 0, 7),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (recursive_wildcard)
{
    char *expression = "$..*";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT_DOT, 2, 1),
        expected_token(ASTERISK, 1, 3),
        expected_token(END_OF_INPUT, 0, 4),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (object_selector)
{
    char *expression = "$.foo.object()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(OBJECT_SELECTOR, 8, 6),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (array_selector)
{
    char *expression = "$.foo.array()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(ARRAY_SELECTOR, 7, 6),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (string_selector)
{
    char *expression = "$.foo.string()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(STRING_SELECTOR, 8, 6),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (number_selector)
{
    char *expression = "$.foo.number()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(NUMBER_SELECTOR, 8, 6),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (integer_selector)
{
    char *expression = "$.foo.integer()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(INTEGER_SELECTOR, 9, 6),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (decimal_selector)
{
    char *expression = "$.foo.decimal()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(DECIMAL_SELECTOR, 9, 6),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (timestamp_selector)
{
    char *expression = "$.foo.timestamp()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(TIMESTAMP_SELECTOR, 11, 6),
        expected_token(END_OF_INPUT, 0, 17),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (boolean_selector)
{
    char *expression = "$.foo.boolean()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(BOOLEAN_SELECTOR, 9, 6),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (null_selector)
{
    char *expression = "$.foo.null()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(NULL_SELECTOR, 6, 6),
        expected_token(END_OF_INPUT, 0, 12),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (wildcard_predicate)
{
    char *expression = "$.foo[*].bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(ASTERISK, 1, 6),
        expected_token(CLOSE_BRACKET, 1, 7),
        expected_token(DOT, 1, 8),
        expected_token(NAME, 3, 9),
        expected_token(END_OF_INPUT, 0, 12),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.foo[2].bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(INTEGER_LITERAL, 1, 6),
        expected_token(CLOSE_BRACKET, 1, 7),
        expected_token(DOT, 1, 8),
        expected_token(NAME, 3, 9),
        expected_token(END_OF_INPUT, 0, 12),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (slice_predicate)
{
    char *expression = "$.foo[:2].bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(COLON, 1, 6),
        expected_token(INTEGER_LITERAL, 1, 7),
        expected_token(CLOSE_BRACKET, 1, 8),
        expected_token(DOT, 1, 9),
        expected_token(NAME, 3, 10),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (slice_predicate_negative_from)
{
    char *expression = "$.foo[-1:].bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(MINUS, 1, 6),
        expected_token(INTEGER_LITERAL, 1, 7),
        expected_token(COLON, 1, 8),
        expected_token(CLOSE_BRACKET, 1, 9),
        expected_token(DOT, 1, 10),
        expected_token(NAME, 3, 11),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (slice_predicate_with_step)
{
    char *expression = "$.foo[:2:2].bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(COLON, 1, 6),
        expected_token(INTEGER_LITERAL, 1, 7),
        expected_token(COLON, 1, 8),
        expected_token(INTEGER_LITERAL, 1, 9),
        expected_token(CLOSE_BRACKET, 1, 10),
        expected_token(DOT, 1, 11),
        expected_token(NAME, 3, 12),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (slice_predicate_copy)
{
    char *expression = "$.foo[::].bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(COLON, 1, 6),
        expected_token(COLON, 1, 7),
        expected_token(CLOSE_BRACKET, 1, 8),
        expected_token(DOT, 1, 9),
        expected_token(NAME, 3, 10),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (slice_predicate_negative_step)
{
    char *expression = "$.foo[::-1].bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(COLON, 1, 6),
        expected_token(COLON, 1, 7),
        expected_token(MINUS, 1, 8),
        expected_token(INTEGER_LITERAL, 1, 9),
        expected_token(CLOSE_BRACKET, 1, 10),
        expected_token(DOT, 1, 11),
        expected_token(NAME, 3, 12),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name)
{
    char *expression = "$.'foo'.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 5, 2),
        expected_token(DOT, 1, 7),
        expected_token(NAME, 3, 8),
        expected_token(END_OF_INPUT, 0, 11),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name_escaped_quote)
{
    char *expression = "$.'fo\\'o'.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 7, 2),
        expected_token(DOT, 1, 9),
        expected_token(NAME, 3, 10),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name_escaped_newline)
{
    char *expression = "$.'fo\\no'.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 7, 2),
        expected_token(DOT, 1, 9),
        expected_token(NAME, 3, 10),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name_escaped_hex)
{
    char *expression = "$.'fo\\xdfo'.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 9, 2),
        expected_token(DOT, 1, 11),
        expected_token(NAME, 3, 12),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name_escaped_utf16)
{
    char *expression = "$.'da \\uD83D\\uDCA9'.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 17, 2),
        expected_token(DOT, 1, 19),
        expected_token(NAME, 3, 20),
        expected_token(END_OF_INPUT, 0, 23),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name_escaped_utf32)
{
    char *expression = "$.'da \\U0001F4A9'.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 15, 2),
        expected_token(DOT, 1, 17),
        expected_token(NAME, 3, 18),
        expected_token(END_OF_INPUT, 0, 21),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name_escaped_buffet)
{
    char *expression = "$.'foo \\\"\\\\\\/\\ \\0\\a\\b\\e\\f\\n\\r\\t\\v\\L\\N\\P bar'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 42, 2),
        expected_token(END_OF_INPUT, 0, 44),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (integer_expression)
{
    char *expression = "5-2";
    Token expectations[] = {
        expected_token(INTEGER_LITERAL, 1, 0),
        expected_token(MINUS, 1, 1),
        expected_token(INTEGER_LITERAL, 1, 2),
        expected_token(END_OF_INPUT, 0, 3),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (real_expression)
{
    char *expression = "5*0.5";
    Token expectations[] = {
        expected_token(INTEGER_LITERAL, 1, 0),
        expected_token(ASTERISK, 1, 1),
        expected_token(REAL_LITERAL, 3, 2),
        expected_token(END_OF_INPUT, 0, 5),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (real_expression_with_shorthand)
{
    char *expression = "5*.5";
    Token expectations[] = {
        expected_token(INTEGER_LITERAL, 1, 0),
        expected_token(ASTERISK, 1, 1),
        expected_token(REAL_LITERAL, 2, 2),
        expected_token(END_OF_INPUT, 0, 4),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (empty_input)
{
    char *expression = "";
    Token expectations[] = {
        expected_token(END_OF_INPUT, 0, 0),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (unquoted_name_escape_dot_attempt)
{
    char *expression = "$.foo\\.bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 4, 2),
        expected_token(DOT, 1, 6),
        expected_token(NAME, 3, 7),
        expected_token(END_OF_INPUT, 0, 10),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (unquoted_name_escape_sequence)
{
    char *expression = "$.foo\\\\bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 8, 2),
        expected_token(END_OF_INPUT, 0, 10),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (unquoted_name_illegal_escape_sequence)
{
    char *expression = "$.foo\\zbar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 8, 2),
        expected_token(END_OF_INPUT, 0, 10),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (expression_with_bare_exponent)
{
    char *expression = "5+e10";
    Token expectations[] = {
        expected_token(INTEGER_LITERAL, 1, 0),
        expected_token(PLUS, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(END_OF_INPUT, 0, 5),
    };
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_expectations(lexer, expectations);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (name_includes_newline)
{
    char *expression = "$.foo\nbar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 7, 2),
        expected_token(END_OF_INPUT, 0, 9),
    };
    LexerError errors[] = {
        (LexerError){UNSUPPORTED_CONTROL_CHARACTER, .position.index=5},
    };    
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_errors(lexer, expectations, errors);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (name_includes_tab)
{
    char *expression = "$.foo\tbar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 7, 2),
        expected_token(END_OF_INPUT, 0, 9),
    };
    LexerError errors[] = {
        (LexerError){UNSUPPORTED_CONTROL_CHARACTER, .position.index=5},
    };    
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_errors(lexer, expectations, errors);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (name_includes_ack)
{
    char *expression = "$.foo\006bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 7, 2),
        expected_token(END_OF_INPUT, 0, 9),
    };
    LexerError errors[] = {
        (LexerError){UNSUPPORTED_CONTROL_CHARACTER, .position.index=5},
    };    
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_errors(lexer, expectations, errors);
    dispose_lexer(lexer);
}
END_TEST

START_TEST (quoted_name_illegal_escape_sequence)
{
    char *expression = "$.'foo\\zbar'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 10, 2),
        expected_token(END_OF_INPUT, 0, 12),
    };
    LexerError errors[] = {
        (LexerError){UNSUPPORTED_ESCAPE_SEQUENCE, .position.index=7},
    };    
    Lexer *lexer = make_lexer(expression, strlen(expression));
    assert_not_null(lexer);
    assert_errors(lexer, expectations, errors);
    dispose_lexer(lexer);
}
END_TEST

Suite *lexer_suite(void)
{
    TCase *expected_case = tcase_create("expected");
    tcase_add_test(expected_case, basic);
    tcase_add_test(expected_case, dotdot);
    tcase_add_test(expected_case, wildcard);
    tcase_add_test(expected_case, recursive_wildcard);
    tcase_add_test(expected_case, object_selector);
    tcase_add_test(expected_case, array_selector);
    tcase_add_test(expected_case, string_selector);
    tcase_add_test(expected_case, number_selector);
    tcase_add_test(expected_case, integer_selector);
    tcase_add_test(expected_case, decimal_selector);
    tcase_add_test(expected_case, timestamp_selector);
    tcase_add_test(expected_case, boolean_selector);
    tcase_add_test(expected_case, null_selector);
    tcase_add_test(expected_case, wildcard_predicate);
    tcase_add_test(expected_case, subscript_predicate);
    tcase_add_test(expected_case, slice_predicate);
    tcase_add_test(expected_case, slice_predicate_negative_from);
    tcase_add_test(expected_case, slice_predicate_with_step);
    tcase_add_test(expected_case, slice_predicate_copy);
    tcase_add_test(expected_case, slice_predicate_negative_step);
    tcase_add_test(expected_case, quoted_name);
    tcase_add_test(expected_case, quoted_name_escaped_quote);
    tcase_add_test(expected_case, quoted_name_escaped_newline);
    tcase_add_test(expected_case, quoted_name_escaped_hex);
    tcase_add_test(expected_case, quoted_name_escaped_utf16);
    tcase_add_test(expected_case, quoted_name_escaped_utf32);
    tcase_add_test(expected_case, quoted_name_escaped_buffet);
    tcase_add_test(expected_case, integer_expression);
    tcase_add_test(expected_case, real_expression);
    tcase_add_test(expected_case, real_expression_with_shorthand);

    TCase *subtleties_case = tcase_create("subtleties");
    tcase_add_test(subtleties_case, empty_input);
    tcase_add_test(subtleties_case, unquoted_name_escape_dot_attempt);
    tcase_add_test(subtleties_case, unquoted_name_escape_sequence);
    tcase_add_test(subtleties_case, unquoted_name_illegal_escape_sequence);
    tcase_add_test(subtleties_case, expression_with_bare_exponent);

    TCase *errors_case = tcase_create("errors");
    tcase_add_test(errors_case, name_includes_newline);
    tcase_add_test(errors_case, name_includes_tab);
    tcase_add_test(errors_case, name_includes_ack);
    tcase_add_test(errors_case, quoted_name_illegal_escape_sequence);

    Suite *suite = suite_create("Lexer");
    suite_add_tcase(suite, expected_case);
    suite_add_tcase(suite, subtleties_case);
    suite_add_tcase(suite, errors_case);

    return suite;
}
