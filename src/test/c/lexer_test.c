#include <stdio.h>

#include "test.h"

// check defines a fail helper that conflicts with the maybe constructor
#undef fail

#include "parser/lexer.h"

#define expected_token(KIND, EXTENT, INDEX) (ExpectedToken){.kind=(KIND), .location.extent=(EXTENT), .location.index=(INDEX), .location.line=0, .location.offset=(INDEX)}

#define assert_just(X, I) ck_assert_msg(is_just((X)), "Assertion 'is_just("#X")' failed.  buffer:1:%d %s", input_index((I)), lexer_strerror(from_nothing((X))))
#define assert_token(E, A) ck_assert_msg((E).kind == (A).kind, "Assertion '"#E".kind == "#A".kind' failed: "#E".kind==%s, "#A".kind==%s", token_name((E).kind), token_name((A).kind)); \
    assert_int_eq(E.location.extent, A.lexeme.location.extent);     \
    assert_int_eq(E.location.index, A.lexeme.location.index);       \
    assert_int_eq(E.location.line, A.lexeme.location.line);         \
    assert_int_eq(E.location.offset, A.lexeme.location.offset)
#define assert_expectations(E, I) for(size_t i = 0; i < sizeof((E))/sizeof(ExpectedToken); i++) \
    {                                                                   \
    Maybe(Token) token = next((I));                                     \
    assert_just(token, I);                                              \
    ExpectedToken expected = (E)[i];                                    \
    assert_token(expected, from_just(token));                           \
    }

struct expected_token_s
{
    enum token_kind kind;
    Location location;    
};

typedef struct expected_token_s ExpectedToken;

START_TEST (basic)
{
    char *expression = "$.foo.bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(NAME, 3, 6),
        expected_token(END_OF_INPUT, 0, 9),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (dotdot)
{
    char *expression = "$.foo..bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT_DOT, 2, 5),
        expected_token(NAME, 3, 7),
        expected_token(END_OF_INPUT, 0, 10),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (wildcard)
{
    char *expression = "$.foo.*";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(ASTERISK, 1, 6),
        expected_token(END_OF_INPUT, 0, 7),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (recursive_wildcard)
{
    char *expression = "$..*";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT_DOT, 2, 1),
        expected_token(ASTERISK, 1, 3),
        expected_token(END_OF_INPUT, 0, 4),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (object_selector)
{
    char *expression = "$.foo.object()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(OBJECT_SELECTOR, 8, 6),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (array_selector)
{
    char *expression = "$.foo.array()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(ARRAY_SELECTOR, 7, 6),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (string_selector)
{
    char *expression = "$.foo.string()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(STRING_SELECTOR, 8, 6),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (number_selector)
{
    char *expression = "$.foo.number()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(NUMBER_SELECTOR, 8, 6),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (integer_selector)
{
    char *expression = "$.foo.integer()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(INTEGER_SELECTOR, 9, 6),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (decimal_selector)
{
    char *expression = "$.foo.decimal()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(DECIMAL_SELECTOR, 9, 6),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (timestamp_selector)
{
    char *expression = "$.foo.timestamp()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(TIMESTAMP_SELECTOR, 11, 6),
        expected_token(END_OF_INPUT, 0, 17),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (boolean_selector)
{
    char *expression = "$.foo.boolean()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(BOOLEAN_SELECTOR, 9, 6),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (null_selector)
{
    char *expression = "$.foo.null()";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(NULL_SELECTOR, 6, 6),
        expected_token(END_OF_INPUT, 0, 12),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (wildcard_predicate)
{
    char *expression = "$.foo[*].bar";
    ExpectedToken expectations[] = {
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
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.foo[2].bar";
    ExpectedToken expectations[] = {
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
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (slice_predicate)
{
    char *expression = "$.foo[:2].bar";
    ExpectedToken expectations[] = {
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
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (slice_predicate_negative_from)
{
    char *expression = "$.foo[-1:].bar";
    ExpectedToken expectations[] = {
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
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (slice_predicate_with_step)
{
    char *expression = "$.foo[:2:2].bar";
    ExpectedToken expectations[] = {
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
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (slice_predicate_copy)
{
    char *expression = "$.foo[::].bar";
    ExpectedToken expectations[] = {
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
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (slice_predicate_negative_step)
{
    char *expression = "$.foo[::-1].bar";
    ExpectedToken expectations[] = {
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
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (quoted_name)
{
    char *expression = "$.'foo'.bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 3, 3),
        expected_token(DOT, 1, 7),
        expected_token(NAME, 3, 8),
        expected_token(END_OF_INPUT, 0, 11),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (quoted_name_escaped_quote)
{
    char *expression = "$.'fo\\'o'.bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 5, 3),
        expected_token(DOT, 1, 9),
        expected_token(NAME, 3, 10),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (quoted_name_escaped_newline)
{
    char *expression = "$.'fo\\no'.bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 5, 3),
        expected_token(DOT, 1, 9),
        expected_token(NAME, 3, 10),
        expected_token(END_OF_INPUT, 0, 13),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (quoted_name_escaped_hex)
{
    char *expression = "$.'fo\\xdfo'.bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 7, 3),
        expected_token(DOT, 1, 11),
        expected_token(NAME, 3, 12),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (quoted_name_escaped_utf16)
{
    char *expression = "$.'da \\uD83D\\uDCA9'.bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 15, 3),
        expected_token(DOT, 1, 19),
        expected_token(NAME, 3, 20),
        expected_token(END_OF_INPUT, 0, 23),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (quoted_name_escaped_utf32)
{
    char *expression = "$.'da \\U0001F4A9'.bar";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 13, 3),
        expected_token(DOT, 1, 17),
        expected_token(NAME, 3, 18),
        expected_token(END_OF_INPUT, 0, 21),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
}
END_TEST

START_TEST (quoted_name_escaped_buffet)
{
    char *expression = "$.'foo \\\"\\\\\\/\\ \\0\\a\\b\\e\\f\\n\\r\\t\\v\\L\\N\\P bar'";
    ExpectedToken expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 40, 3),
        expected_token(END_OF_INPUT, 0, 44),
    };
    Input *input = make_input_from_buffer(expression, strlen(expression));
    assert_not_null(input);
    assert_expectations(expectations, input);
    dispose_input(input);
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

    Suite *suite = suite_create("Lexer");
    suite_add_tcase(suite, expected_case);

    return suite;
}
