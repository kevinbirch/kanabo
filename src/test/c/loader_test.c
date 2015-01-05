/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 *
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README file in the project root.
 *
 * Distributed under an [MIT-style][license] license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimers in the documentation and/or
 *   other materials provided with the distribution.
 * - Neither the names of the copyright holders, nor the names of the authors, nor
 *   the names of other contributors may be used to endorse or promote products
 *   derived from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/ncsa
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <check.h>

#include "loader.h"
#include "loader/private.h"
#include "test.h"
#include "test_model.h"

static const unsigned char * const ALIAS_LOOP_YAML = (unsigned char *)
    "level1: &id001\n"
    "  key1-1: foo\n"
    "  key1-2: bar\n"
    "  level2:\n"
    "    key2-1: foo\n"
    "    key2-2: bar\n"
    "    level3:\n"
    "      - *id001\n"
    "      - bar1\n";

static const unsigned char * const NON_SCALAR_KEY_YAML = (unsigned char *)
    "foo: 1\n"
    "? - one\n"
    "  - two\n"
    ":\n"
    "  - foo1\n"
    "  - bar1\n";

static const unsigned char * const MISSING_ANCHOR_YAML = (unsigned char *)
    "one:\n"
    "  - foo1\n"
    "  - bar1\n"
    "\n"
    "two: *value\n";

static const unsigned char * const YAML = (unsigned char *)
    "one:\n"
    "  - foo1\n"
    "  - bar1\n"
    "\n"
    "two: \"foo2\"\n"
    "\n"
    "three: null\n"
    "\n"
    "four:\n"
    "  - true\n"
    "  - false\n"
    "\n"
    "five:\n"
    "  - 1.5\n"
    "  - 42\n"
    "  - 1978-07-26 10:15";

static const unsigned char * const TAGGED_YAML = (unsigned char *)
    "%TAG !squid! tag:vampire-squid.com,2008:\n"
    "--- !squid!instrument\n"
    "name: !!str USD-JPY 2008-04-01 103.92\n"
    "asset-class: !squid!asset-class FX\n"
    "type: !squid!instrument/type spot market\n"
    "exchange-rate: !!float 103.92\n"
    "symbol: !squid!instrument/symbol USD-JPY\n"
    "spot-date: !!timestamp 2008-04-01T10:12:00Z\n"
    "settlement-date: !!timestamp 2008-04-03T09:00:00Z\n";

static const unsigned char * const ANCHOR_YAML = (unsigned char *)
    "one:\n"
    "  - &value foo1\n"
    "  - &value bar1\n"
    "\n"
    "two: *value\n";

static const unsigned char * const KEY_ANCHOR_YAML = (unsigned char *)
    "&value one:\n"
    "  - foo1\n"
    "  - *value\n"
    "two: *value\n";

static const unsigned char * const DUPLICATE_KEY_YAML = (unsigned char *)
    "one: foo\n"
    "two:\n"
    "  - foo\n"
    "  - bar\n"
    "one: bar\n"
    "three: baz\n";


static DocumentModel *model_fixture = NULL;
static Node *root_node_fixture = NULL;

#define assert_loader_failure(MAYBE, EXPECTED_RESULT)                   \
    do                                                                  \
    {                                                                   \
        assert_int_eq(NOTHING, (MAYBE).tag);                            \
        assert_int_eq((EXPECTED_RESULT), (MAYBE).nothing.code);         \
        assert_not_null((MAYBE).nothing.message);                       \
        free((MAYBE).nothing.message);                                  \
    } while(0)


static void model_setup(const unsigned char *data, size_t length, enum loader_duplicate_key_strategy value)
{
    MaybeDocument maybe = load_string(data, length, value);
    assert_not_null(maybe.just);
    model_fixture = maybe.just;

    reset_errno();
    assert_uint_eq(1, model_size(model_fixture));

    reset_errno();
    root_node_fixture = model_document_root(model_fixture, 0);
    assert_noerr();
    assert_not_null(root_node_fixture);
}

static void model_teardown(void)
{
    model_free(model_fixture);
    model_fixture = NULL;
    root_node_fixture = NULL;
}

START_TEST (null_string_input)
{
    reset_errno();
    MaybeDocument maybe = load_string(NULL, 50, DUPE_CLOBBER);
    assert_errno(EINVAL);

    assert_loader_failure(maybe, ERR_INPUT_IS_NULL);
}
END_TEST

START_TEST (zero_string_input_length)
{
    reset_errno();
    MaybeDocument maybe = load_string((unsigned char *)"", 0, DUPE_CLOBBER);
    assert_errno(EINVAL);

    assert_loader_failure(maybe, ERR_INPUT_SIZE_IS_ZERO);
}
END_TEST

START_TEST (null_file_input)
{
    reset_errno();
    MaybeDocument maybe = load_file(NULL, DUPE_CLOBBER);
    assert_errno(EINVAL);

    assert_loader_failure(maybe, ERR_INPUT_IS_NULL);
}
END_TEST

START_TEST (eof_file_input)
{
    FILE *input = tmpfile();
    fseek(input, 0, SEEK_END);

    reset_errno();
    MaybeDocument maybe = load_file(input, DUPE_CLOBBER);
    assert_errno(EINVAL);

    assert_loader_failure(maybe, ERR_INPUT_SIZE_IS_ZERO);

    fclose(input);
}
END_TEST

START_TEST (non_scalar_key)
{
    size_t yaml_size = strlen((char *)NON_SCALAR_KEY_YAML);

    MaybeDocument maybe = load_string(NON_SCALAR_KEY_YAML, yaml_size, DUPE_CLOBBER);
    assert_loader_failure(maybe, ERR_NON_SCALAR_KEY);
}
END_TEST

START_TEST (alias_loop)
{
    size_t yaml_size = strlen((char *)ALIAS_LOOP_YAML);

    MaybeDocument maybe = load_string(ALIAS_LOOP_YAML, yaml_size, DUPE_CLOBBER);
    assert_loader_failure(maybe, ERR_ALIAS_LOOP);
}
END_TEST

START_TEST (missing_anchor)
{
    size_t yaml_size = strlen((char *)MISSING_ANCHOR_YAML);

    MaybeDocument maybe = load_string(MISSING_ANCHOR_YAML, yaml_size, DUPE_CLOBBER);
    assert_loader_failure(maybe, ERR_NO_ANCHOR_FOR_ALIAS);
}
END_TEST

static void assert_model_state(DocumentModel *model)
{
    assert_not_null(model);
    assert_uint_eq(1, model_size(model));

    reset_errno();
    Node *root = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(root);

    assert_node_kind(root, MAPPING);
    assert_node_size(root, 5);

    reset_errno();
    Node *one = mapping_get(mapping(root), (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);

    reset_errno();
    Node *one_0 = sequence_get(sequence(one), 0);
    assert_noerr();
    assert_node_kind(one_0, SCALAR);
    assert_scalar_value((one_0), "foo1");
    assert_scalar_kind(one_0, SCALAR_STRING);
    reset_errno();
    Node *one_1 = sequence_get(sequence(one), 1);
    assert_noerr();
    assert_node_kind(one_1, SCALAR);
    assert_scalar_value((one_1), "bar1");
    assert_scalar_kind(one_1, SCALAR_STRING);

    reset_errno();
    Node *two = mapping_get(mapping(root), (uint8_t *)"two", 3ul);
    assert_noerr();
    assert_not_null(two);
    assert_node_kind(two, SCALAR);
    assert_scalar_value((two), "foo2");
    assert_scalar_kind(two, SCALAR_STRING);

    reset_errno();
    Node *three = mapping_get(mapping(root), (uint8_t *)"three", 5ul);
    assert_noerr();
    assert_not_null(three);
    assert_node_kind(three, SCALAR);
    assert_scalar_value((three), "null");
    assert_scalar_kind(three, SCALAR_NULL);

    reset_errno();
    Node *four = mapping_get(mapping(root), (uint8_t *)"four", 4ul);
    assert_noerr();
    assert_not_null(four);
    assert_node_kind(four, SEQUENCE);

    reset_errno();
    Node *four_0 = sequence_get(sequence(four), 0);
    assert_noerr();
    assert_node_kind(four_0, SCALAR);
    assert_scalar_value((four_0), "true");
    assert_scalar_kind(four_0, SCALAR_BOOLEAN);
    assert_true(scalar_boolean_is_true(scalar(four_0)));
    assert_false(scalar_boolean_is_false(scalar(four_0)));
    reset_errno();
    Node *four_1 = sequence_get(sequence(four), 1);
    assert_noerr();
    assert_node_kind(four_0, SCALAR);
    assert_scalar_value((four_1), "false");
    assert_scalar_kind(four_1, SCALAR_BOOLEAN);
    assert_true(scalar_boolean_is_false(scalar(four_1)));
    assert_false(scalar_boolean_is_true(scalar(four_1)));

    reset_errno();
    Node *five = mapping_get(mapping(root), (uint8_t *)"five", 4ul);
    assert_noerr();
    assert_not_null(five);
    assert_node_kind(five, SEQUENCE);
    Node *five_0 = sequence_get(sequence(five), 0);
    assert_noerr();
    assert_node_kind(five_0, SCALAR);
    assert_scalar_value((five_0), "1.5");
    assert_scalar_kind(five_0, SCALAR_REAL);
    reset_errno();
    Node *five_1 = sequence_get(sequence(five), 1);
    assert_noerr();
    assert_node_kind(five_1, SCALAR);
    assert_scalar_value((five_1), "42");
    assert_scalar_kind(five_1, SCALAR_INTEGER);
    reset_errno();
    Node *five_2 = sequence_get(sequence(five), 2);
    assert_noerr();
    assert_node_kind(five_2, SCALAR);
    assert_scalar_value(five_2, "1978-07-26 10:15");
    assert_scalar_kind(five_2, SCALAR_TIMESTAMP);
    reset_errno();
}

START_TEST (load_from_file)
{
    size_t yaml_size = strlen((char *)YAML);

    FILE *input = tmpfile();
    size_t written = fwrite(YAML, sizeof(char), yaml_size, input);
    assert_uint_eq(written, yaml_size);
    int ret = fflush(input);
    assert_int_eq(0, ret);

    rewind(input);

    MaybeDocument maybe = load_file(input, DUPE_CLOBBER);
    assert_int_eq(JUST, maybe.tag);
    assert_not_null(maybe.just);

    assert_model_state(maybe.just);

    fclose(input);
    model_free(maybe.just);
}
END_TEST

START_TEST (load_from_string)
{
    size_t yaml_size = strlen((char *)YAML);

    MaybeDocument maybe = load_string(YAML, yaml_size, DUPE_CLOBBER);
    assert_int_eq(JUST, maybe.tag);
    assert_not_null(maybe.just);

    assert_model_state(maybe.just);

    model_free(maybe.just);
}
END_TEST

static void tagged_yaml_setup(void)
{
    model_setup(TAGGED_YAML, strlen((char *)TAGGED_YAML), DUPE_CLOBBER);

    assert_node_kind(root_node_fixture, MAPPING);
    assert_node_size(root_node_fixture, 7);
}

START_TEST (shorthand_tags)
{
    reset_errno();
    assert_node_tag(root_node_fixture, "tag:vampire-squid.com,2008:instrument");

    Mapping *root = mapping(root_node_fixture);

    reset_errno();
    Node *asset_class = mapping_get(root, (uint8_t *)"asset-class", 11ul);
    assert_noerr();
    assert_not_null(asset_class);
    assert_node_kind(asset_class, SCALAR);
    assert_scalar_kind(asset_class, SCALAR_STRING);
    assert_node_tag(asset_class, "tag:vampire-squid.com,2008:asset-class");

    reset_errno();
    Node *type = mapping_get(root, (uint8_t *)"type", 4ul);
    assert_noerr();
    assert_not_null(type);
    assert_node_kind(type, SCALAR);
    assert_scalar_kind(type, SCALAR_STRING);
    assert_node_tag(type, "tag:vampire-squid.com,2008:instrument/type");

    reset_errno();
    Node *symbol = mapping_get(root, (uint8_t *)"symbol", 6ul);
    assert_noerr();
    assert_not_null(symbol);
    assert_node_kind(symbol, SCALAR);
    assert_scalar_kind(symbol, SCALAR_STRING);
    assert_node_tag(symbol, "tag:vampire-squid.com,2008:instrument/symbol");
}
END_TEST

START_TEST (explicit_tags)
{
    Mapping *root = mapping(root_node_fixture);

    reset_errno();
    Node *name = mapping_get(root, (uint8_t *)"name", 4ul);
    assert_noerr();
    assert_not_null(name);
    assert_node_kind(name, SCALAR);
    assert_scalar_kind(name, SCALAR_STRING);
    assert_node_tag(name, "tag:yaml.org,2002:str");

    reset_errno();
    Node *exchange_rate = mapping_get(root, (uint8_t *)"exchange-rate", 13ul);
    assert_noerr();
    assert_not_null(exchange_rate);
    assert_node_kind(exchange_rate, SCALAR);
    assert_scalar_kind(exchange_rate, SCALAR_REAL);
    assert_node_tag(exchange_rate, "tag:yaml.org,2002:float");

    reset_errno();
    Node *spot_date = mapping_get(root, (uint8_t *)"spot-date", 9ul);
    assert_noerr();
    assert_not_null(spot_date);
    assert_node_kind(spot_date, SCALAR);
    assert_scalar_kind(spot_date, SCALAR_TIMESTAMP);
    assert_node_tag(spot_date, "tag:yaml.org,2002:timestamp");

    reset_errno();
    Node *settlement_date = mapping_get(root, (uint8_t *)"settlement-date", 15ul);
    assert_noerr();
    assert_not_null(settlement_date);
    assert_node_kind(settlement_date, SCALAR);
    assert_scalar_kind(settlement_date, SCALAR_TIMESTAMP);
    assert_node_tag(settlement_date, "tag:yaml.org,2002:timestamp");
}
END_TEST

static void anchor_yaml_setup(void)
{
    model_setup(ANCHOR_YAML, strlen((char *)ANCHOR_YAML), DUPE_CLOBBER);

    assert_node_kind(root_node_fixture, MAPPING);
    assert_node_size(root_node_fixture, 2);
}

START_TEST (anchor)
{
    Mapping *root = mapping(root_node_fixture);

    reset_errno();
    Node *one = mapping_get(root, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);

    reset_errno();
    Node *a = mapping_get(root, (uint8_t *)"two", 3ul);
    assert_noerr();
    assert_not_null(a);
    assert_node_kind(a, ALIAS);

    Node *two = alias_target(alias(a));
    assert_not_null(two);
    assert_node_kind(two, SCALAR);
    assert_scalar_kind(two, SCALAR_STRING);
    assert_scalar_value(two, "bar1");
}
END_TEST

static void key_anchor_yaml_setup(void)
{
    model_setup(KEY_ANCHOR_YAML, strlen((char *)KEY_ANCHOR_YAML), DUPE_CLOBBER);

    assert_node_kind(root_node_fixture, MAPPING);
    assert_node_size(root_node_fixture, 2);
}

START_TEST (key_anchor)
{
    Mapping *root = mapping(root_node_fixture);

    reset_errno();
    Node *one = mapping_get(root, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);

    reset_errno();
    Node *alias1 = sequence_get(sequence(one), 1);
    assert_noerr();
    assert_not_null(alias1);
    assert_node_kind(alias1, ALIAS);

    Node *one_1 = alias_target(alias(alias1));
    assert_noerr();
    assert_not_null(one_1);
    assert_node_kind(one_1, SCALAR);
    assert_scalar_kind(one_1, SCALAR_STRING);
    assert_scalar_value(one_1, "one");
    reset_errno();

    Node *alias2 = mapping_get(root, (uint8_t *)"two", 3ul);
    assert_noerr();
    assert_not_null(alias2);

    Node *two = alias_target(alias(alias2));
    assert_node_kind(two, SCALAR);
    assert_scalar_kind(two, SCALAR_STRING);
    assert_scalar_value(two, "one");
}
END_TEST

static void duplicate_setup(enum loader_duplicate_key_strategy value)
{
    model_setup(DUPLICATE_KEY_YAML, strlen((char *)DUPLICATE_KEY_YAML), value);

    assert_node_kind(root_node_fixture, MAPPING);
    assert_node_size(root_node_fixture, 3);
}

static void duplicate_clobber_setup(void)
{
    duplicate_setup(DUPE_CLOBBER);
}

START_TEST (duplicate_clobber)
{
    reset_errno();
    Node *one = mapping_get(mapping(root_node_fixture), (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SCALAR);
    assert_scalar_value(one, "bar");
    assert_scalar_kind(one, SCALAR_STRING);
}
END_TEST

static void duplicate_warn_setup(void)
{
    duplicate_setup(DUPE_WARN);
}

START_TEST (duplicate_warn)
{
    reset_errno();
    Node *one = mapping_get(mapping(root_node_fixture), (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SCALAR);
    assert_scalar_value(one, "bar");
    assert_scalar_kind(one, SCALAR_STRING);
}
END_TEST

START_TEST (duplicate_fail)
{
    size_t yaml_size = strlen((char *)DUPLICATE_KEY_YAML);

    MaybeDocument maybe = load_string(DUPLICATE_KEY_YAML, yaml_size, DUPE_FAIL);
    assert_loader_failure(maybe, ERR_DUPLICATE_KEY);
}
END_TEST

Suite *loader_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_string_input);
    tcase_add_test(bad_input_case, zero_string_input_length);
    tcase_add_test(bad_input_case, null_file_input);
    tcase_add_test(bad_input_case, eof_file_input);
    tcase_add_test(bad_input_case, non_scalar_key);
    tcase_add_test(bad_input_case, alias_loop);
    tcase_add_test(bad_input_case, missing_anchor);

    TCase *file_case = tcase_create("file");
    tcase_add_test(file_case, load_from_file);

    TCase *string_case = tcase_create("string");
    tcase_add_test(string_case, load_from_string);

    TCase *tag_case = tcase_create("tag");
    tcase_add_unchecked_fixture(tag_case, tagged_yaml_setup, model_teardown);
    tcase_add_test(tag_case, shorthand_tags);
    tcase_add_test(tag_case, explicit_tags);

    TCase *anchor_case = tcase_create("anchor");
    tcase_add_unchecked_fixture(anchor_case, anchor_yaml_setup, model_teardown);
    tcase_add_test(anchor_case, anchor);

    TCase *key_anchor_case = tcase_create("key_anchor");
    tcase_add_unchecked_fixture(key_anchor_case, key_anchor_yaml_setup, model_teardown);
    tcase_add_test(key_anchor_case, key_anchor);

    TCase *duplicate_clobber_case = tcase_create("duplicate_clobber");
    tcase_add_unchecked_fixture(duplicate_clobber_case, duplicate_clobber_setup, model_teardown);
    tcase_add_test(duplicate_clobber_case, duplicate_clobber);

    TCase *duplicate_warn_case = tcase_create("duplicate_warn_clobber");
    tcase_add_unchecked_fixture(duplicate_warn_case, duplicate_warn_setup, model_teardown);
    tcase_add_test(duplicate_warn_case, duplicate_warn);

    TCase *duplicate_fail_case = tcase_create("duplicate_fail_clobber");
    tcase_add_test(duplicate_fail_case, duplicate_fail);

    Suite *loader = suite_create("Loader");
    suite_add_tcase(loader, bad_input_case);
    suite_add_tcase(loader, file_case);
    suite_add_tcase(loader, string_case);
    suite_add_tcase(loader, tag_case);
    suite_add_tcase(loader, anchor_case);
    suite_add_tcase(loader, duplicate_clobber_case);
    suite_add_tcase(loader, duplicate_warn_case);
    suite_add_tcase(loader, duplicate_fail_case);

    return loader;
}
