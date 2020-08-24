#include <stdio.h>

#include "vector.h"
#include "xalloc.h"

#include "test.h"

#include "parser/scanner.h"

#define expected_token(KIND, EXTENT, INDEX) (Token){.kind=(KIND), .location.start.index=(INDEX), .location.start.line=0, .location.start.offset=(INDEX), .location.end.index=(INDEX)+(EXTENT), .location.end.line=0, .location.end.offset=(INDEX)+(EXTENT)}

#define assert_token(E, A) \
    do {                                                                \
        ck_assert_msg(E.kind == A.kind, "Assertion '"#E".kind == "#A".kind' failed: "#E".kind==%s, "#A".kind==%s", token_name(E.kind), token_name(A.kind)); \
        assert_uint_eq(E.location.start.index, A.location.start.index); \
        assert_uint_eq(E.location.end.index, A.location.end.index);     \
        assert_uint_eq(E.location.start.line, A.location.start.line);   \
        assert_uint_eq(E.location.end.line, A.location.end.line);       \
        assert_uint_eq(E.location.start.offset, A.location.start.offset); \
        assert_uint_eq(E.location.end.offset, A.location.end.offset);   \
    } while(0)

#define assert_expectations(L, E)                                       \
    do {                                                                \
        for(size_t i = 0; i < sizeof(E)/sizeof(Token); i++)             \
        {                                                               \
            scanner_next(L);                                            \
            assert_token(E[i], (L)->current);                           \
        }                                                               \
        assert_true(vector_is_empty((L)->errors));                      \
    } while(0)

#define assert_errors(L, X, E) \
    do {                                                                \
        for(size_t i = 0; i < sizeof(X)/sizeof(Token); i++)             \
        {                                                               \
            scanner_next(L);                                            \
            assert_token(X[i], (L)->current);                           \
        }                                                               \
        assert_false(vector_is_empty((L)->errors));                     \
        size_t count = sizeof(E)/sizeof(ParserError);                   \
        assert_uint_eq(count, vector_length((L)->errors));              \
        for(size_t i = 0; i < count; i++)                               \
        {                                                               \
            ParserError *err = vector_get((L)->errors, i);              \
            assert_not_null(err);                                       \
            printf("i: %zu, err: %s, \n", i, parser_strerror(err->code)); \
            ck_assert_msg(E[i].code == err->code, "Assertion '"#E"[%zu].code == err->code' failed: "#E"[%zu].code==\"%s\", err->code==\"%s\"", i, i, parser_strerror(E[i].code), parser_strerror(err->code)); \
            assert_uint_eq(E[i].location.start.index, err->location.start.index); \
            assert_uint_eq(E[i].location.start.line, err->location.start.line); \
            assert_uint_eq(E[i].location.start.offset, err->location.start.offset); \
            assert_uint_eq(E[i].location.end.index, err->location.end.index); \
            assert_uint_eq(E[i].location.end.line, err->location.end.line); \
            assert_uint_eq(E[i].location.end.offset, err->location.end.offset); \
            assert_uint_eq(E[i].index, err->index);                     \
        }                                                               \
    } while(0)

#define make_error(CODE, START, END, POINT) (ParserError){.code=(CODE), .location.start.index=(START), .location.start.line=0, .location.start.offset=(START), .location.end.index=(END), .location.end.line=0, .location.end.offset=(END), .index=(POINT)}

#define make_scanner(EXP, LEN) {.errors=make_vector_with_capacity(1), .input=make_input_from_buffer((EXP), (LEN))}

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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
        expected_token(INTEGER_LITERAL, 2, 6),
        expected_token(COLON, 1, 8),
        expected_token(CLOSE_BRACKET, 1, 9),
        expected_token(DOT, 1, 10),
        expected_token(NAME, 3, 11),
        expected_token(END_OF_INPUT, 0, 14),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
        expected_token(INTEGER_LITERAL, 2, 8),
        expected_token(CLOSE_BRACKET, 1, 10),
        expected_token(DOT, 1, 11),
        expected_token(NAME, 3, 12),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_escaped_buffet)
{
    char *expression = "$.'foo \\\"\\\\\\/\\ \\_\\0\\a\\b\\e\\f\\n\\r\\t\\v\\L\\N\\P bar'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 44, 2),
        expected_token(END_OF_INPUT, 0, 46),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

/* START_TEST (integer_expression) */
/* { */
/*     char *expression = "5-2"; */
/*     Token expectations[] = { */
/*         expected_token(INTEGER_LITERAL, 1, 0), */
/*         expected_token(MINUS, 1, 1), */
/*         expected_token(INTEGER_LITERAL, 1, 2), */
/*         expected_token(END_OF_INPUT, 0, 3), */
/*     }; */
/*     Parser parser = make_scanner(expression, strlen(expression)); */
/*     assert_expectations(&parser, expectations); */
/*     parser_release(&parser); */
/* } */
/* END_TEST */

START_TEST (real_expression)
{
    char *expression = "5*0.5";
    Token expectations[] = {
        expected_token(INTEGER_LITERAL, 1, 0),
        expected_token(ASTERISK, 1, 1),
        expected_token(REAL_LITERAL, 3, 2),
        expected_token(END_OF_INPUT, 0, 5),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (root_step_predicated)
{
    char *expression = "$[1].foo";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(OPEN_BRACKET, 1, 1),
        expected_token(INTEGER_LITERAL, 1, 2),
        expected_token(CLOSE_BRACKET, 1, 3),
        expected_token(DOT, 1, 4),
        expected_token(NAME, 3, 5),
        expected_token(END_OF_INPUT, 0, 8),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (relative_path_explicit)
{
    char *expression = "@foo.bar";
    Token expectations[] = {
        expected_token(AT, 1, 0),
        expected_token(NAME, 3, 1),
        expected_token(DOT, 1, 4),
        expected_token(NAME, 3, 5),
        expected_token(END_OF_INPUT, 0, 8),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (relative_path_explicit_predicated)
{
    char *expression = "@[1].foo";
    Token expectations[] = {
        expected_token(AT, 1, 0),
        expected_token(OPEN_BRACKET, 1, 1),
        expected_token(INTEGER_LITERAL, 1, 2),
        expected_token(CLOSE_BRACKET, 1, 3),
        expected_token(DOT, 1, 4),
        expected_token(NAME, 3, 5),
        expected_token(END_OF_INPUT, 0, 8),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (transfomer_step)
{
    char *expression = "$.foo.={\"name\": name}";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(EQUALS, 1, 6),
        expected_token(OPEN_BRACE, 1, 7),
        expected_token(STRING_LITERAL, 6, 8),
        expected_token(COLON, 1, 14),
        expected_token(NAME, 4, 16),
        expected_token(CLOSE_BRACE, 1, 20),
        expected_token(END_OF_INPUT, 0, 21),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (tag_selector)
{
    char *expression = "$.foo.!bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(EXCLAMATION, 1, 6),
        expected_token(NAME, 3, 7),
        expected_token(END_OF_INPUT, 0, 10),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (anchor_selector)
{
    char *expression = "$.foo.&bar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(AMPERSAND, 1, 6),
        expected_token(NAME, 3, 7),
        expected_token(END_OF_INPUT, 0, 10),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (join_predicate)
{
    char *expression = "$.foo[1, -1]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_BRACKET, 1, 5),
        expected_token(INTEGER_LITERAL, 1, 6),
        expected_token(COMMA, 1, 7),
        expected_token(INTEGER_LITERAL, 2, 9),
        expected_token(CLOSE_BRACKET, 1, 11),
        expected_token(END_OF_INPUT, 0, 12),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate)
{
    char *expression = "$.foo[?value > 1]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(NAME, 5, 7),
        expected_token(GREATER_THAN, 1, 13),
        expected_token(INTEGER_LITERAL, 1, 15),
        expected_token(CLOSE_BRACKET, 1, 16),
        expected_token(END_OF_INPUT, 0, 17),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_parenthesized)
{
    char *expression = "$.foo[?(value > 1)]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 5, 8),
        expected_token(GREATER_THAN, 1, 14),
        expected_token(INTEGER_LITERAL, 1, 16),
        expected_token(CLOSE_PARENTHESIS, 1, 17),
        expected_token(CLOSE_BRACKET, 1, 18),
        expected_token(END_OF_INPUT, 0, 19),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_equals_null)
{
    char *expression = "$.foo[?(name = null)]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 4, 8),
        expected_token(EQUALS, 1, 13),
        expected_token(NULL_LITERAL, 4, 15),
        expected_token(CLOSE_PARENTHESIS, 1, 19),
        expected_token(CLOSE_BRACKET, 1, 20),
        expected_token(END_OF_INPUT, 0, 21),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_path_gt_integer)
{
    char *expression = "$.foo[?(some.thing > 1)]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 4, 8),
        expected_token(DOT, 1, 12),
        expected_token(NAME, 5, 13),
        expected_token(GREATER_THAN, 1, 19),
        expected_token(INTEGER_LITERAL, 1, 21),
        expected_token(CLOSE_PARENTHESIS, 1, 22),
        expected_token(CLOSE_BRACKET, 1, 23),
        expected_token(END_OF_INPUT, 0, 24),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_path_expr_gte_path_expr)
{
    char *expression = "$.foo[?(this.thing >= that.thing)]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 4, 8),
        expected_token(DOT, 1, 12),
        expected_token(NAME, 5, 13),
        expected_token(GREATER_THAN_EQUAL, 2, 19),
        expected_token(NAME, 4, 22),
        expected_token(DOT, 1, 26),
        expected_token(NAME, 5, 27),
        expected_token(CLOSE_PARENTHESIS, 1, 32),
        expected_token(CLOSE_BRACKET, 1, 33),
        expected_token(END_OF_INPUT, 0, 34),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_path_lt_integer)
{
    char *expression = "$.foo[?(value < 1)]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 5, 8),
        expected_token(LESS_THAN, 1, 14),
        expected_token(INTEGER_LITERAL, 1, 16),
        expected_token(CLOSE_PARENTHESIS, 1, 17),
        expected_token(CLOSE_BRACKET, 1, 18),
        expected_token(END_OF_INPUT, 0, 19),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_path_eq_string)
{
    char *expression = "$.foo[?(this.thing = \"string\")]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 4, 8),
        expected_token(DOT, 1, 12),
        expected_token(NAME, 5, 13),
        expected_token(EQUALS, 1, 19),
        expected_token(STRING_LITERAL, 8, 21),
        expected_token(CLOSE_PARENTHESIS, 1, 29),
        expected_token(CLOSE_BRACKET, 1, 30),
        expected_token(END_OF_INPUT, 0, 31),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_path_ne_bool)
{
    char *expression = "$.foo[?(value != false)]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 5, 8),
        expected_token(NOT_EQUAL, 2, 14),
        expected_token(BOOLEAN_LITERAL_FALSE, 5, 17),
        expected_token(CLOSE_PARENTHESIS, 1, 22),
        expected_token(CLOSE_BRACKET, 1, 23),
        expected_token(END_OF_INPUT, 0, 24),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (filter_predicate_multiple_bool_expr)
{
    char *expression = "$.foo[?(value > 1 and name != null)]";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(OPEN_FILTER, 2, 5),
        expected_token(OPEN_PARENTHESIS, 1, 7),
        expected_token(NAME, 5, 8),
        expected_token(GREATER_THAN, 1, 14),
        expected_token(INTEGER_LITERAL, 1, 16),
        expected_token(BOOLEAN_AND, 3, 18),
        expected_token(NAME, 4, 22),
        expected_token(NOT_EQUAL, 2, 27),
        expected_token(NULL_LITERAL, 4, 30),
        expected_token(CLOSE_PARENTHESIS, 1, 34),
        expected_token(CLOSE_BRACKET, 1, 35),
        expected_token(END_OF_INPUT, 0, 36),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
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
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (type_selector_interstitial_whitespace)
{
    char *expression = "$.foo.object ()";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(NAME, 3, 2),
        expected_token(DOT, 1, 5),
        expected_token(NAME, 6, 6),
        expected_token(OPEN_PARENTHESIS, 1, 13),
        expected_token(CLOSE_PARENTHESIS, 1, 14),
        expected_token(END_OF_INPUT, 0, 15),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_expectations(&parser, expectations);
    parser_release(&parser);
}
END_TEST

START_TEST (integer_eoi_exponent)
{
    char *expression = "5e";
    Token expectations[] = {
        expected_token(INTEGER_LITERAL, 2, 0),
        expected_token(END_OF_INPUT, 0, 2),
    };
    ParserError expected_errors[] = {
        make_error(PREMATURE_END_OF_INPUT, 0, 2, 2)
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (real_eoi_fraction)
{
    char *expression = "5.";
    Token expectations[] = {
        expected_token(REAL_LITERAL, 2, 0),
        expected_token(END_OF_INPUT, 0, 2),
    };
    ParserError expected_errors[] = {
        make_error(PREMATURE_END_OF_INPUT, 0, 2, 2),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (real_eoi_exponent)
{
    char *expression = "5.1e";
    Token expectations[] = {
        expected_token(REAL_LITERAL, 4, 0),
        expected_token(END_OF_INPUT, 0, 4),
    };
    ParserError expected_errors[] = {
        make_error(PREMATURE_END_OF_INPUT, 0, 4, 4),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (name_includes_newline)
{
    char *expression = "$.foo\nbar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        (Token){.kind=NAME, .location.start.index=2, .location.start.line=0, .location.start.offset=2, .location.end.index=9, .location.end.line=1, .location.end.offset=3},
        (Token){.kind=END_OF_INPUT, .location.start.index=9, .location.start.line=1, .location.start.offset=3, .location.end.index=9, .location.end.line=1, .location.end.offset=3},
    };
    ParserError expected_errors[] = {
        (ParserError){.code=UNSUPPORTED_CONTROL_CHARACTER, .location.start.index=2, .location.start.line=0, .location.start.offset=2, .location.end.index=9, .location.end.line=1, .location.end.offset=3, .index=5}
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
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
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_CONTROL_CHARACTER, 2, 9, 5),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
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
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_CONTROL_CHARACTER, 2, 9, 5),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
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
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 12, 7)
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_illegal_hex_escape_sequence)
{
    char *expression = "$.'fo\\xobar'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 10, 2),
        expected_token(END_OF_INPUT, 0, 12),
    };
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 11, 7),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_short_hex_escape_sequence)
{
    char *expression = "$.'foo\\xdmbar'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 12, 2),
        expected_token(END_OF_INPUT, 0, 14),
    };
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 12, 9),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_eoi_hex_escape_sequence)
{
    char *expression = "$.'foo\\x";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 6, 2),
        expected_token(END_OF_INPUT, 0, 8),
    };
    ParserError expected_errors[] = {
        make_error(PREMATURE_END_OF_INPUT, 8, 8, 8),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_illegal_utf16_escape_sequence)
{
    char *expression = "$.'foo\\umbar'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 11, 2),
        expected_token(END_OF_INPUT, 0, 13),
    };
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 12, 8),
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 12, 11),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_short_utf16_escape_sequence)
{
    char *expression = "$.'foo\\ueemm'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 11, 2),
        expected_token(END_OF_INPUT, 0, 13),
    };
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 11, 10),
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 11, 11),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_eoi_utf16_escape_sequence)
{
    char *expression = "$.'foo\\uee";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 8, 2),
        expected_token(END_OF_INPUT, 0, 10),
    };
    ParserError expected_errors[] = {
        make_error(PREMATURE_END_OF_INPUT, 10, 10, 10),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_illegal_utf32_escape_sequence)
{
    char *expression = "$.'foo\\Udeadbxxf'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 15, 2),
        expected_token(END_OF_INPUT, 0, 17),
    };
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 16, 13),
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 16, 14),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_short_utf32_escape_sequence)
{
    char *expression = "$.'foo\\Udeadmmmm'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 15, 2),
        expected_token(END_OF_INPUT, 0, 17),
    };
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 16, 12),
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 16, 13),
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 16, 14),
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 16, 15),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_eoi_utf32_escape_sequence)
{
    char *expression = "$.'foo\\Udead";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 10, 2),
        expected_token(END_OF_INPUT, 0, 12),
    };
    ParserError expected_errors[] = {
        make_error(PREMATURE_END_OF_INPUT, 12, 12, 12),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_multiple_illegal_escape_sequence)
{
    char *expression = "$.'fo\\zob\\mar'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 12, 2),
        expected_token(END_OF_INPUT, 0, 14),
    };
    ParserError expected_errors[] = {
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 14, 6),
        make_error(UNSUPPORTED_ESCAPE_SEQUENCE, 2, 14, 10),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_unterminated_literal)
{
    char *expression = "$.'foobar";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 7, 2),
        expected_token(END_OF_INPUT, 0, 9),
    };
    ParserError expected_errors[] = {
        make_error(UNCLOSED_QUOTATION, 2, 9, 2),
        make_error(PREMATURE_END_OF_INPUT, 2, 9, 9),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_eoi)
{
    char *expression = "$.'";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 1, 2),
        expected_token(END_OF_INPUT, 0, 3),
    };
    ParserError expected_errors[] = {
        make_error(UNCLOSED_QUOTATION, 2, 3, 2),
        make_error(PREMATURE_END_OF_INPUT, 3, 3, 3),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

START_TEST (quoted_name_eoi_escape_sequence)
{
    char *expression = "$.'foobar\\";
    Token expectations[] = {
        expected_token(DOLLAR, 1, 0),
        expected_token(DOT, 1, 1),
        expected_token(QUOTED_NAME, 8, 2),
        expected_token(END_OF_INPUT, 0, 10),
    };
    ParserError expected_errors[] = {
        make_error(PREMATURE_END_OF_INPUT, 10, 10, 10),
    };
    Parser parser = make_scanner(expression, strlen(expression));
    assert_errors(&parser, expectations, expected_errors);
    parser_release(&parser);
}
END_TEST

Suite *scanner_suite(void)
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
//    tcase_add_test(expected_case, integer_expression);
    tcase_add_test(expected_case, real_expression);
    tcase_add_test(expected_case, real_expression_with_shorthand);
    tcase_add_test(expected_case, root_step_predicated);
    tcase_add_test(expected_case, relative_path_explicit);
    tcase_add_test(expected_case, relative_path_explicit_predicated);
    tcase_add_test(expected_case, transfomer_step);
    tcase_add_test(expected_case, tag_selector);
    tcase_add_test(expected_case, anchor_selector);
    tcase_add_test(expected_case, join_predicate);
    tcase_add_test(expected_case, filter_predicate);
    tcase_add_test(expected_case, filter_predicate_parenthesized);
    tcase_add_test(expected_case, filter_predicate_equals_null);
    tcase_add_test(expected_case, filter_predicate_path_gt_integer);
    tcase_add_test(expected_case, filter_predicate_path_expr_gte_path_expr);
    tcase_add_test(expected_case, filter_predicate_path_lt_integer);
    tcase_add_test(expected_case, filter_predicate_path_eq_string);
    tcase_add_test(expected_case, filter_predicate_path_ne_bool);
    tcase_add_test(expected_case, filter_predicate_multiple_bool_expr);

    TCase *subtleties_case = tcase_create("subtleties");
    tcase_add_test(subtleties_case, name_includes_tab);
    tcase_add_test(subtleties_case, name_includes_ack);
    tcase_add_test(subtleties_case, name_includes_newline);
    tcase_add_test(subtleties_case, unquoted_name_escape_dot_attempt);
    tcase_add_test(subtleties_case, unquoted_name_escape_sequence);
    tcase_add_test(subtleties_case, unquoted_name_illegal_escape_sequence);
    tcase_add_test(subtleties_case, expression_with_bare_exponent);
    tcase_add_test(subtleties_case, type_selector_interstitial_whitespace);

    TCase *errors_case = tcase_create("errors");
    tcase_add_test(errors_case, integer_eoi_exponent);
    tcase_add_test(errors_case, real_eoi_fraction);
    tcase_add_test(errors_case, real_eoi_exponent);
    tcase_add_test(errors_case, quoted_name_illegal_escape_sequence);
    tcase_add_test(errors_case, quoted_name_multiple_illegal_escape_sequence);
    tcase_add_test(errors_case, quoted_name_eoi);
    tcase_add_test(errors_case, quoted_name_unterminated_literal);
    tcase_add_test(errors_case, quoted_name_eoi_escape_sequence);
    tcase_add_test(errors_case, quoted_name_illegal_hex_escape_sequence);
    tcase_add_test(errors_case, quoted_name_short_hex_escape_sequence);
    tcase_add_test(errors_case, quoted_name_eoi_hex_escape_sequence);
    tcase_add_test(errors_case, quoted_name_illegal_utf16_escape_sequence);
    tcase_add_test(errors_case, quoted_name_short_utf16_escape_sequence);
    tcase_add_test(errors_case, quoted_name_eoi_utf16_escape_sequence);
    tcase_add_test(errors_case, quoted_name_illegal_utf32_escape_sequence);
    tcase_add_test(errors_case, quoted_name_short_utf32_escape_sequence);
    tcase_add_test(errors_case, quoted_name_eoi_utf32_escape_sequence);

    Suite *suite = suite_create("Scanner");
    suite_add_tcase(suite, expected_case);
    suite_add_tcase(suite, subtleties_case);
    suite_add_tcase(suite, errors_case);

    return suite;
}
