#include "test.h"

#include "parser.h"

#define assert_parser_success(M, K, L)                                  \
    do                                                                  \
    {                                                                   \
        assert_true(is_just(M));                                        \
        assert_int_eq(from_just((M))->kind, (K));                       \
        assert_uint_eq(vector_length(from_just((M))->steps), (L));      \
    } while(0)

#define assert_parser_failure(M, E)                                     \
    do                                                                  \
    {                                                                   \
        assert_true(is_nothing(M));                                     \
        Vector *nothing = from_nothing(M);                              \
        assert_false(vector_is_empty(nothing));                         \
        size_t count = sizeof(E)/sizeof(ParserError);                   \
        assert_uint_eq(vector_length(nothing), count);                  \
        for(size_t i = 0; i < count; i++)                               \
        {                                                               \
            ParserError *err = vector_get(nothing, i);                  \
            assert_not_null(err);                                       \
            ck_assert_msg(E[i].code == err->code, "Assertion '"#E"[%zu].code == err->code' failed: "#E"[%zu].code==\"%s\", err->code==\"%s\"", i, i, parser_strerror(E[i].code), parser_strerror(err->code)); \
            assert_uint_eq(E[i].position.index, err->position.index);   \
        }                                                               \
    } while(0)

#define path_get(PATH, INDEX) ((Step *)vector_get((PATH)->steps, INDEX))

#define assert_step_kind(STEP, EXPECTED_KIND) assert_int_eq((EXPECTED_KIND), (STEP)->kind)
#define assert_test_kind(STEP, EXPECTED_KIND) assert_int_eq((EXPECTED_KIND), (STEP)->test.kind)

#define assert_step(PATH, INDEX, EXPECTED_STEP_KIND, EXPECTED_TEST_KIND) \
    assert_step_kind(path_get((PATH), (INDEX)), (EXPECTED_STEP_KIND));   \
    assert_test_kind(path_get((PATH), (INDEX)), (EXPECTED_TEST_KIND))

#define assert_name_length(STEP, NAME) assert_uint_eq(strlen((NAME)), name_test_step_length((STEP)))
#define assert_name(STEP, NAME)                                         \
    assert_name_length((STEP), (NAME));                                 \
    assert_buf_eq((NAME), strlen((NAME)), name_test_step_name((STEP)), name_test_step_length((STEP)))

#define assert_no_predicate(PATH, INDEX)                             \
    assert_null(path_get((PATH), (INDEX))->predicate)

#define assert_root_step(PATH)                  \
    assert_step((PATH), 0, ROOT, NAME_TEST);    \
    assert_no_predicate((PATH), 0)

#define assert_name_step(PATH,INDEX, NAME, EXPECTED_STEP_KIND)      \
    assert_step((PATH), (INDEX), (EXPECTED_STEP_KIND), NAME_TEST);   \
    assert_name(path_get((PATH), (INDEX)), (NAME))
#define assert_single_name_step(PATH, INDEX, NAME) assert_name_step((PATH), (INDEX), (NAME), SINGLE)
#define assert_recursive_name_step(PATH, INDEX, NAME) assert_name_step((PATH), (INDEX), (NAME), RECURSIVE)

#define assert_wildcard_step(PATH, INDEX, EXPECTED_STEP_KIND) assert_step((PATH), (INDEX), (EXPECTED_STEP_KIND), WILDCARD_TEST)
#define assert_single_wildcard_step(PATH, INDEX) assert_wildcard_step((PATH), (INDEX), SINGLE)
#define assert_recursive_wildcard_step(PATH, INDEX) assert_wildcard_step((PATH), (INDEX), RECURSIVE)

#define assert_type_kind(STEP, EXPECTED) assert_int_eq((EXPECTED), (STEP)->test.type)

#define assert_type_step(PATH, INDEX, EXPECTED_TYPE_KIND, EXPECTED_STEP_KIND) \
    assert_step((PATH), (INDEX), (EXPECTED_STEP_KIND), TYPE_TEST);      \
    assert_type_kind(path_get((PATH), INDEX), (EXPECTED_TYPE_KIND))

#define assert_single_type_step(PATH, INDEX, EXPECTED_TYPE_KIND)    \
    assert_type_step((PATH), (INDEX), (EXPECTED_TYPE_KIND), SINGLE)
#define assert_recursive_type_step(PATH, INDEX, EXPECTED_TYPE_KIND) \
    assert_type_step((PATH), (INDEX), (EXPECTED_TYPE_KIND), RECURSIVE)

#define assert_predicate_kind(PREDICATE, EXPECTED) assert_int_eq((EXPECTED), (PREDICATE)->kind)

#define assert_predicate(PATH, PATH_INDEX, EXPECTED_PREDICATE_KIND)     \
    assert_not_null(path_get((PATH), (PATH_INDEX))->predicate);         \
    assert_predicate_kind(path_get((PATH), (PATH_INDEX))->predicate, (EXPECTED_PREDICATE_KIND))

#define assert_wildcard_predicate(PATH, PATH_INDEX) assert_predicate((PATH), (PATH_INDEX), (WILDCARD))

#define assert_subscript_index(PREDICATE, VALUE) assert_int_eq((VALUE), (PREDICATE)->subscript.index)
#define assert_subscript_predicate(PATH, PATH_INDEX, INDEX_VALUE)       \
    assert_predicate((PATH), (PATH_INDEX), SUBSCRIPT);                  \
    assert_subscript_index(path_get((PATH), (PATH_INDEX))->predicate, (INDEX_VALUE));

#define assert_slice_from(PREDICATE, VALUE) assert_int_eq((VALUE), (PREDICATE)->slice.from)
#define assert_slice_to(PREDICATE, VALUE) assert_int_eq((VALUE), (PREDICATE)->slice.to)
#define assert_slice_step(PREDICATE, VALUE) assert_int_eq((VALUE), (PREDICATE)->slice.step)
#define assert_slice_predicate(PATH, PATH_INDEX, FROM_VALUE, TO_VALUE, STEP_VALUE) \
    assert_predicate((PATH), (PATH_INDEX), (SLICE));                    \
    assert_slice_from(path_get((PATH), (PATH_INDEX))->predicate, (FROM_VALUE)); \
    assert_slice_to(path_get((PATH), (PATH_INDEX))->predicate, (TO_VALUE)); \
    assert_slice_step(path_get((PATH), (PATH_INDEX))->predicate, (STEP_VALUE))

#define print_errors(MAYBE)                                             \
    for(size_t i = 0; i < vector_length(from_nothing(MAYBE)); i++)      \
    {                                                                   \
        ParserError *err = (ParserError *)vector_get(from_nothing(MAYBE), i); \
        log_error(tcase_name(), "at %zu error: %s", err->position.index, parser_strerror(err->code)); \
    }

#define print_steps(MAYBE)                                              \
    for(size_t i = 0; i < vector_length(from_just(MAYBE).steps); i++)   \
    {                                                                   \
        Step *step = (Step *)vector_get(from_just(MAYBE).steps, i);     \
        log_error(tcase_name(), "step %zu: %s", i, step_kind_name(step->kind)); \
    }

static inline void dispose_maybe(Maybe(JsonPath) maybe)
{
    if(is_nothing(maybe))
    {
        vector_destroy(from_nothing(maybe), free);
    }
    else
    {
        dispose_path(from_just(maybe));
    }
}

START_TEST (null_expression)
{
    char *expression = NULL;
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EMPTY_INPUT, .position.index=0},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (zero_length)
{
    char *expression = "";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EMPTY_INPUT, .position.index=0},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (missing_step_test)
{
    char *expression = "$.";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_STEP_PRODUCTION, .position.index=2},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (missing_recursive_step_test)
{
    char *expression = "$..";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_STEP_PRODUCTION, .position.index=3},
    };
    
    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (missing_dot)
{
    char *expression = "$x";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=1},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (unclosed_empty_root_predicate)
{
    char *expression = "$[";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNBALANCED_PRED_DELIM, .position.index=1},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (stray_root_predicate_closure)
{
    char *expression = "$]";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=1},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (empty_root_predicate)
{
    char *expression = "$[].bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_PREDICATE_PRODUCTION, .position.index=2},
    };
    
    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (tripple_troubble)
{
    char *expression = "$...foo";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_STEP_PRODUCTION, .position.index=3},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (tripple_troubble_redux)
{
    char *expression = "$.foo...bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_STEP_PRODUCTION, .position.index=7},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (tripple_troubble_trilux)
{
    char *expression = "$.foo...bar.baz";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_STEP_PRODUCTION, .position.index=7},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (premature_unclosed_quoted_step)
{
    char *expression = "$.foo.'";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNCLOSED_QUOTATION, .position.index=6},
        (ParserError){PREMATURE_END_OF_INPUT, .position.index=7},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (unclosed_quoted_step)
{
    char *expression = "$.foo.'bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNCLOSED_QUOTATION, .position.index=6},
        (ParserError){PREMATURE_END_OF_INPUT, .position.index=10},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (unclosed_escaped_quoted_step)
{
    char *expression = "$.foo.'bar\\'";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNCLOSED_QUOTATION, .position.index=6},
        (ParserError){PREMATURE_END_OF_INPUT, .position.index=12},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (empty_predicate)
{
    char *expression = "$.foo[].bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_PREDICATE_PRODUCTION, .position.index=6},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (extra_junk_in_predicate)
{
    char *expression = "$.foo[ * quux].bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNEXPECTED_INPUT, .position.index=9},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (whitespace_predicate)
{
    char *expression = "$.foo[ \t ].bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_PREDICATE_PRODUCTION, .position.index=9},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_predicate)
{
    char *expression = "$.foo[!!].bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_PREDICATE_PRODUCTION, .position.index=6},
        (ParserError){UNEXPECTED_INPUT, .position.index=7},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_type_test_name)
{
    char *expression = "$.foo.monkey()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=12},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_oblong)
{
    char *expression = "$.foo.oblong()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=12},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_alloy)
{
    char *expression = "$.foo.alloy()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=11},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_strong)
{
    char *expression = "$.foo.strong()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=12},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_numbered)
{
    char *expression = "$.foo.numbered()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=14},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_booger)
{
    char *expression = "$.foo.booger()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=12},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (bogus_type_test_name_narl)
{
    char *expression = "$.foo.narl()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=10},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (empty_type_test_name)
{
    char *expression = "$.foo.()";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_STEP_PRODUCTION, .position.index=6},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (dollar_only)
{
    char *expression = "$";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 1);
    assert_root_step(from_just(maybe));

    dispose_maybe(maybe);
}
END_TEST

START_TEST (absolute_single_step)
{
    char *expression = "$.foo";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 2);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_no_predicate(from_just(maybe), 0);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (absolute_recursive_step)
{
    char *expression = "$..foo";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 2);
    assert_root_step(from_just(maybe));
    assert_recursive_name_step(from_just(maybe), 1, "foo");
    assert_no_predicate(from_just(maybe), 0);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (absolute_multi_step)
{
    char *expression = "$.foo.baz..yobble.thingum";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 5);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_name_step(from_just(maybe), 2, "baz");
    assert_recursive_name_step(from_just(maybe), 3, "yobble");
    assert_single_name_step(from_just(maybe), 4, "thingum");
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);
    assert_no_predicate(from_just(maybe), 3);
    assert_no_predicate(from_just(maybe), 4);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (relative_path_begins_with_dot)
{
    char *expression = ".x";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, RELATIVE_PATH, 1);
    assert_single_name_step(from_just(maybe), 0, "x");
    assert_no_predicate(from_just(maybe), 0);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (relative_multi_step)
{
    char *expression = "foo.bar..baz";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, RELATIVE_PATH, 3);
    assert_single_name_step(from_just(maybe), 0, "foo");
    assert_single_name_step(from_just(maybe), 1, "bar");
    assert_recursive_name_step(from_just(maybe), 2, "baz");
    assert_no_predicate(from_just(maybe), 0);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (quoted_empty_step)
{
    char *expression = "$.foo.''.bar";
    Maybe(JsonPath) maybe = parse(expression);

    // xxx - assert success
    dispose_maybe(maybe);
}
END_TEST

START_TEST (quoted_escape_step)
{
    char *expression = "$.foo.'mon\\'key'.bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 4);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_name_step(from_just(maybe), 2, "mon'key");
    assert_single_name_step(from_just(maybe), 3, "bar");
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);
    assert_no_predicate(from_just(maybe), 3);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (quoted_multi_step)
{
    char *expression = "$.foo.'happy fun ball'.bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 4);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_name_step(from_just(maybe), 2, "happy fun ball");
    assert_single_name_step(from_just(maybe), 3, "bar");
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);
    assert_no_predicate(from_just(maybe), 3);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (wildcard)
{
    char *expression = "$.foo.*";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_wildcard_step(from_just(maybe), 2);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (recursive_wildcard)
{
    char *expression = "$.foo..*";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_recursive_wildcard_step(from_just(maybe), 2);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (wildcard_with_subscript_predicate)
{
    char *expression = "$.foo.* [0]";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_no_predicate(from_just(maybe), 1);
    assert_single_wildcard_step(from_just(maybe), 2);
    assert_subscript_predicate(from_just(maybe), 2, 0);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (escape_artistry)
{
    char *expression = "$.'foo\\_\\L\\N\\Pzap'";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 2);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo\xC2\xA0\xE2\x80\xA8\xC2\x85\xE2\x80\xA9zap");
    assert_no_predicate(from_just(maybe), 1);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (hex_escape)
{
    char *expression = "$.'foo \\xc2 bar'";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 2);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo \xC2 bar");
    assert_no_predicate(from_just(maybe), 1);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (ucs2_escape)
{
    char *expression = "$.'foo \\u2603 bar'";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 2);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo \xe2\x98\x83 bar");
    assert_no_predicate(from_just(maybe), 1);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (ucs4_escape)
{
    char *expression = "$.'foo \\U0001f4a9 bar'";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 2);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo \xf0\x9f\x92\xa9 bar");
    assert_no_predicate(from_just(maybe), 1);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (ucs2_surrogate)
{
    char *expression = "$.'foo \\ud800 bar'";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNSUPPORTED_UNICODE_SEQUENCE, .position.index=2},
    };
    print_errors(maybe);

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (ucs2_non_character)
{
    char *expression = "$.'foo \\ufffe bar'";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNSUPPORTED_UNICODE_SEQUENCE, .position.index=2},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (ucs4_overflow)
{
    char *expression = "$.'foo \\U00110000 bar'";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){UNSUPPORTED_UNICODE_SEQUENCE, .position.index=2},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (whitespace)
{
    char *expression = "  $ \r\n. foo \n.. \t'happy fun ball' . \t string()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 4);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_recursive_name_step(from_just(maybe), 2, "happy fun ball");
    assert_single_type_step(from_just(maybe), 3, STRING_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);
    assert_no_predicate(from_just(maybe), 3);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (type_test_missing_closing_paren)
{
    char *expression = "$.foo.object(";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){EXPECTED_QUALIFIED_STEP_PRODUCTION, .position.index=12},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (recursive_type_test)
{
    char *expression = "$.foo..string()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_recursive_type_step(from_just(maybe), 2, STRING_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (object_type_test)
{
    char *expression = "$.foo.object()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_type_step(from_just(maybe), 2, OBJECT_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (array_type_test)
{
    char *expression = "$.foo.array()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_type_step(from_just(maybe), 2, ARRAY_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (string_type_test)
{
    char *expression = "$.foo.string()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_type_step(from_just(maybe), 2, STRING_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (number_type_test)
{
    char *expression = "$.foo.number()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_type_step(from_just(maybe), 2, NUMBER_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (boolean_type_test)
{
    char *expression = "$.foo.boolean()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_type_step(from_just(maybe), 2, BOOLEAN_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (null_type_test)
{
    char *expression = "$.foo.null()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_single_type_step(from_just(maybe), 2, NULL_TEST);
    assert_no_predicate(from_just(maybe), 1);
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (wildcard_predicate)
{
    char *expression = "$.store.book[*].author";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 4);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "store");
    assert_no_predicate(from_just(maybe), 1);
    assert_single_name_step(from_just(maybe), 2, "book");
    assert_wildcard_predicate(from_just(maybe), 2);
    assert_single_name_step(from_just(maybe), 3, "author");
    assert_no_predicate(from_just(maybe), 3);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (wildcard_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t*\n]  .bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_wildcard_predicate(from_just(maybe), 1);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.foo[42].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_subscript_predicate(from_just(maybe), 1, 42);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (subscript_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t42\r]\n.bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_subscript_predicate(from_just(maybe), 1, 42);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (type_test_with_subscript_predicate)
{
    char *expression = "$.foo.array()[0]";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_no_predicate(from_just(maybe), 1);
    assert_single_type_step(from_just(maybe), 2, ARRAY_TEST);
    assert_subscript_predicate(from_just(maybe), 2, 0);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (negative_subscript_predicate)
{
    char *expression = "$.foo[ -3].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_subscript_predicate(from_just(maybe), 1, -3);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (slice_predicate_form1)
{
    char *expression = "$.foo[:-3].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, 0, -3, 0);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (slice_predicate_form1_with_step)
{
    char *expression = "$.foo[:-3:2].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, 0, -3, 2);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (slice_predicate_form2)
{
    char *expression = "$.foo[-3:].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, -3, 0, 0);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (slice_predicate_form2_with_step)
{
    char *expression = "$.foo[-1::2].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, -1, 0, 2);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (slice_predicate_form3)
{
    char *expression = "$.foo[3:5].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, 3, 5, 0);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (slice_predicate_form3_with_step)
{
    char *expression = "$.foo[1:4:2].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, 1, 4, 2);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (slice_predicate_with_whitespace)
{
    char *expression = "$.foo  [\t1\t:\t5\r:\n3\t]\n.bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, 1, 5, 3);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (negative_step_slice_predicate)
{
    char *expression = "$.foo[1:3:-3].bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);
    assert_root_step(from_just(maybe));
    assert_single_name_step(from_just(maybe), 1, "foo");
    assert_slice_predicate(from_just(maybe), 1, 1, 3, -3);
    assert_single_name_step(from_just(maybe), 2, "bar");
    assert_no_predicate(from_just(maybe), 2);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (zero_step_slice_predicate)
{
    char *expression = "$.foo[::0].bar";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){STEP_CANNOT_BE_ZERO, .position.index=8},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (integer_overlflow)
{
    char *expression = "$.foo[9223372036854775808]";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){INTEGER_TOO_BIG, .position.index=6},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST

START_TEST (integer_underflow)
{
    char *expression = "$.foo[-9223372036854775809]";
    Maybe(JsonPath) maybe = parse(expression);
    ParserError errors[] = {
        (ParserError){INTEGER_TOO_SMALL, .position.index=6},
    };

    assert_parser_failure(maybe, errors);
    dispose_maybe(maybe);
}
END_TEST
    
static bool count(Step *each, void *context)
{
    unsigned long *counter = (unsigned long *)context;
    (*counter)++;
    return true;
}

START_TEST (iteration)
{
    char *expression = "$.foo.bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);

    unsigned long counter = 0;
    assert_true(path_iterate(from_just(maybe), count, &counter));
    assert_uint_eq(3, counter);

    dispose_maybe(maybe);
}
END_TEST

static bool fail_count(Step *each, void *context)
{
    unsigned long *counter = (unsigned long *)context;
    if(0 == *counter)
    {
        (*counter)++;
        return true;
    }
    return false;
}

START_TEST (fail_iteration)
{
    char *expression = "$.foo.bar";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);

    unsigned long counter = 0;
    assert_false(path_iterate(from_just(maybe), fail_count, &counter));
    assert_uint_eq(1, counter);

    dispose_maybe(maybe);
}
END_TEST

START_TEST (bad_path_input)
{
    char *expression = "$";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 1);

    assert_null(path_get(from_just(maybe), 1));

    dispose_maybe(maybe);
}
END_TEST

START_TEST (bad_step_input)
{
    char *expression = "$.foo.array()";
    Maybe(JsonPath) maybe = parse(expression);

    assert_parser_success(maybe, ABSOLUTE_PATH, 3);

    Step *step2 = path_get(from_just(maybe), 2);
    assert_uint_eq(0, name_test_step_length(step2));

    assert_null(name_test_step_name(step2));

    dispose_maybe(maybe);
}
END_TEST

START_TEST (bad_predicate_input)
{
    assert_null(join_predicate_left(NULL));

    assert_null(join_predicate_right(NULL));

    char *expression = "$.foo[42].bar[*]";
    Maybe(JsonPath) maybe = parse(expression);
    assert_parser_success(maybe, ABSOLUTE_PATH, 3);

    dispose_maybe(maybe);
}
END_TEST

Suite *jsonpath_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_expression);
    tcase_add_test(bad_input_case, zero_length);
    tcase_add_test(bad_input_case, missing_step_test);
    tcase_add_test(bad_input_case, missing_recursive_step_test);
    tcase_add_test(bad_input_case, missing_dot);
    tcase_add_test(bad_input_case, unclosed_empty_root_predicate);
    tcase_add_test(bad_input_case, stray_root_predicate_closure);
    tcase_add_test(bad_input_case, empty_root_predicate);
    tcase_add_test(bad_input_case, tripple_troubble);
    tcase_add_test(bad_input_case, tripple_troubble_redux);
    tcase_add_test(bad_input_case, tripple_troubble_trilux);
    tcase_add_test(bad_input_case, premature_unclosed_quoted_step);
    tcase_add_test(bad_input_case, unclosed_quoted_step);
    tcase_add_test(bad_input_case, unclosed_escaped_quoted_step);
    tcase_add_test(bad_input_case, bogus_type_test_name);
    tcase_add_test(bad_input_case, bogus_type_test_name_oblong);
    tcase_add_test(bad_input_case, bogus_type_test_name_alloy);
    tcase_add_test(bad_input_case, bogus_type_test_name_strong);
    tcase_add_test(bad_input_case, bogus_type_test_name_numbered);
    tcase_add_test(bad_input_case, bogus_type_test_name_booger);
    tcase_add_test(bad_input_case, bogus_type_test_name_narl);
    tcase_add_test(bad_input_case, empty_type_test_name);
    tcase_add_test(bad_input_case, empty_predicate);
    tcase_add_test(bad_input_case, whitespace_predicate);
    tcase_add_test(bad_input_case, extra_junk_in_predicate);
    tcase_add_test(bad_input_case, bogus_predicate);

    TCase *basic_case = tcase_create("basic");
    tcase_add_test(basic_case, dollar_only);
    tcase_add_test(basic_case, absolute_single_step);
    tcase_add_test(basic_case, absolute_recursive_step);
    tcase_add_test(basic_case, absolute_multi_step);
    tcase_add_test(basic_case, quoted_empty_step);
    tcase_add_test(basic_case, quoted_escape_step);
    tcase_add_test(basic_case, quoted_multi_step);
    tcase_add_test(basic_case, relative_path_begins_with_dot);
    tcase_add_test(basic_case, relative_multi_step);
    tcase_add_test(basic_case, whitespace);
    tcase_add_test(basic_case, wildcard);
    tcase_add_test(basic_case, recursive_wildcard);
    tcase_add_test(basic_case, wildcard_with_subscript_predicate);
    tcase_add_test(basic_case, escape_artistry);
    tcase_add_test(basic_case, hex_escape);
    tcase_add_test(basic_case, ucs2_escape);
    tcase_add_test(basic_case, ucs4_escape);
    tcase_add_test(basic_case, ucs2_surrogate);
    tcase_add_test(basic_case, ucs2_non_character);
    tcase_add_test(basic_case, ucs4_overflow);

    TCase *node_type_case = tcase_create("node type test");
    tcase_add_test(node_type_case, type_test_missing_closing_paren);
    tcase_add_test(node_type_case, recursive_type_test);
    tcase_add_test(node_type_case, object_type_test);
    tcase_add_test(node_type_case, array_type_test);
    tcase_add_test(node_type_case, string_type_test);
    tcase_add_test(node_type_case, number_type_test);
    tcase_add_test(node_type_case, boolean_type_test);
    tcase_add_test(node_type_case, null_type_test);

    TCase *predicate_case = tcase_create("predicate");
    tcase_add_test(predicate_case, wildcard_predicate);
    tcase_add_test(predicate_case, wildcard_predicate_with_whitespace);
    tcase_add_test(predicate_case, subscript_predicate);
    tcase_add_test(predicate_case, subscript_predicate_with_whitespace);
    tcase_add_test(predicate_case, type_test_with_subscript_predicate);
    tcase_add_test(predicate_case, negative_subscript_predicate);
    tcase_add_test(predicate_case, slice_predicate_form1);
    tcase_add_test(predicate_case, slice_predicate_form1_with_step);
    tcase_add_test(predicate_case, slice_predicate_form2);
    tcase_add_test(predicate_case, slice_predicate_form2_with_step);
    tcase_add_test(predicate_case, slice_predicate_form3);
    tcase_add_test(predicate_case, slice_predicate_form3_with_step);
    tcase_add_test(predicate_case, slice_predicate_with_whitespace);
    tcase_add_test(predicate_case, negative_step_slice_predicate);
    tcase_add_test(predicate_case, zero_step_slice_predicate);
    tcase_add_test(predicate_case, integer_overlflow);
    tcase_add_test(predicate_case, integer_underflow);

    TCase *api_case = tcase_create("api");
    tcase_add_test(api_case, bad_path_input);
    tcase_add_test(api_case, bad_step_input);
    tcase_add_test(api_case, bad_predicate_input);
    tcase_add_test(api_case, iteration);
    tcase_add_test(api_case, fail_iteration);

    Suite *suite = suite_create("Parser");
    suite_add_tcase(suite, bad_input_case);
    suite_add_tcase(suite, basic_case);
    suite_add_tcase(suite, node_type_case);
    suite_add_tcase(suite, predicate_case);
    suite_add_tcase(suite, api_case);

    return suite;
}
