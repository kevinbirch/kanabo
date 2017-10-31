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
#include "test_document.h"

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


static document_model *tagged_model = NULL;
static node *tagged_mapping_root = NULL;

static void assert_model_state(loader_context *loader, document_model *model);

void tag_setup(void);
void tag_teardown(void);

document_model *anchor_setup(const unsigned char *yaml);
document_model *duplicate_setup(enum loader_duplicate_key_strategy value);

#define assert_loader_failure(CONTEXT, EXPECTED_RESULT)                 \
    do                                                                  \
    {                                                                   \
        assert_not_null((CONTEXT));                                     \
        assert_int_eq((EXPECTED_RESULT), loader_status((CONTEXT)));     \
        char *_assert_message = loader_status_message((CONTEXT));       \
        assert_not_null(_assert_message);                               \
        log_info("loader test", "received expected failure message: '%s'", _assert_message); \
        free(_assert_message);                                          \
    } while(0)

START_TEST (null_string_input)
{
    reset_errno();
    loader_context *loader = make_string_loader(NULL, 50);
    assert_not_null(loader);
    assert_errno(EINVAL);

    assert_loader_failure(loader, ERR_INPUT_IS_NULL);

    loader_free(loader);
}
END_TEST

START_TEST (zero_string_input_length)
{
    reset_errno();
    loader_context *loader = make_string_loader((unsigned char *)"", 0);
    assert_not_null(loader);
    assert_errno(EINVAL);

    assert_loader_failure(loader, ERR_INPUT_SIZE_IS_ZERO);

    loader_free(loader);
}
END_TEST

START_TEST (null_file_input)
{
    reset_errno();
    loader_context *loader = make_file_loader(NULL);
    assert_not_null(loader);
    assert_errno(EINVAL);

    assert_loader_failure(loader, ERR_INPUT_IS_NULL);    

    loader_free(loader);
}
END_TEST

START_TEST (eof_file_input)
{
    FILE *input = tmpfile();
    fseek(input, 0, SEEK_END);

    reset_errno();
    loader_context *loader = make_file_loader(input);
    assert_not_null(loader);
    assert_errno(EINVAL);

    assert_loader_failure(loader, ERR_INPUT_SIZE_IS_ZERO);    

    fclose(input);
    loader_free(loader);
}
END_TEST

START_TEST (null_context)
{
    reset_errno();
    assert_null(load(NULL));
    assert_errno(EINVAL);    
}
END_TEST

START_TEST (null_context_parser)
{
    loader_context *loader = (loader_context *)calloc(1, sizeof(loader_context));
    loader->parser = NULL;

    reset_errno();
    assert_null(load(loader));
    assert_errno(EINVAL);    

    free(loader);
}
END_TEST

START_TEST (non_scalar_key)
{
    size_t yaml_size = strlen((char *)NON_SCALAR_KEY_YAML);

    loader_context *loader = make_string_loader(NON_SCALAR_KEY_YAML, yaml_size);
    assert_not_null(loader);
    assert_null(load(loader));

    assert_loader_failure(loader, ERR_NON_SCALAR_KEY);

    loader_free(loader);
}
END_TEST

START_TEST (alias_loop)
{
    size_t yaml_size = strlen((char *)ALIAS_LOOP_YAML);

    loader_context *loader = make_string_loader(ALIAS_LOOP_YAML, yaml_size);
    assert_not_null(loader);
    assert_null(load(loader));

    assert_loader_failure(loader, ERR_ALIAS_LOOP);

    loader_free(loader);
}
END_TEST

START_TEST (missing_anchor)
{
    size_t yaml_size = strlen((char *)MISSING_ANCHOR_YAML);

    loader_context *loader = make_string_loader(MISSING_ANCHOR_YAML, yaml_size);
    assert_not_null(loader);
    assert_null(load(loader));

    assert_loader_failure(loader, ERR_NO_ANCHOR_FOR_ALIAS);

    loader_free(loader);
}
END_TEST

START_TEST (load_from_file)
{
    size_t yaml_size = strlen((char *)YAML);

    FILE *input = tmpfile();
    size_t written = fwrite(YAML, sizeof(char), yaml_size, input);
    assert_uint_eq(written, yaml_size);
    int ret = fflush(input);
    assert_int_eq(0, ret);

    rewind(input);

    loader_context *loader = make_file_loader(input);    
    assert_not_null(loader);
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));
    document_model *model = load(loader);
    assert_not_null(model);

    assert_model_state(loader, model);

    fclose(input);
    model_free(model);
    loader_free(loader);
}
END_TEST

START_TEST (load_from_string)
{
    size_t yaml_size = strlen((char *)YAML);

    loader_context *loader = make_string_loader(YAML, yaml_size);
    assert_not_null(loader);
    document_model *model = load(loader);
    assert_not_null(model);
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));

    assert_model_state(loader, model);

    model_free(model);
    loader_free(loader);
}
END_TEST

static void assert_model_state(loader_context *loader, document_model *model)
{
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));
    assert_not_null(model);
    assert_uint_eq(1, model_document_count(model));

    reset_errno();
    node *root = model_document_root(model, 0);
    assert_noerr();
    assert_not_null(root);
    
    assert_node_kind(root, MAPPING);
    assert_node_size(root, 5);

    reset_errno();
    node *one = mapping_get(root, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);

    reset_errno();
    node *one_0 = sequence_get(one, 0);
    assert_noerr();
    assert_node_kind(one_0, SCALAR);
    assert_scalar_value(one_0, "foo1");
    assert_scalar_kind(one_0, SCALAR_STRING);
    reset_errno();
    node *one_1 = sequence_get(one, 1);
    assert_noerr();
    assert_node_kind(one_1, SCALAR);
    assert_scalar_value(one_1, "bar1");
    assert_scalar_kind(one_1, SCALAR_STRING);

    reset_errno();
    node *two = mapping_get(root, (uint8_t *)"two", 3ul);
    assert_noerr();
    assert_not_null(two);
    assert_node_kind(two, SCALAR);
    assert_scalar_value(two, "foo2");
    assert_scalar_kind(two, SCALAR_STRING);

    reset_errno();
    node *three = mapping_get(root, (uint8_t *)"three", 5ul);
    assert_noerr();
    assert_not_null(three);
    assert_node_kind(three, SCALAR);
    assert_scalar_value(three, "null");
    assert_scalar_kind(three, SCALAR_NULL);

    reset_errno();
    node *four = mapping_get(root, (uint8_t *)"four", 4ul);
    assert_noerr();
    assert_not_null(four);
    assert_node_kind(four, SEQUENCE);

    reset_errno();
    node *four_0 = sequence_get(four, 0);
    assert_noerr();
    assert_node_kind(four_0, SCALAR);
    assert_scalar_value(four_0, "true");
    assert_scalar_kind(four_0, SCALAR_BOOLEAN);
    assert_true(scalar_boolean_is_true(four_0));
    assert_false(scalar_boolean_is_false(four_0));
    reset_errno();
    node *four_1 = sequence_get(four, 1);
    assert_noerr();
    assert_node_kind(four_0, SCALAR);
    assert_scalar_value(four_1, "false");
    assert_scalar_kind(four_1, SCALAR_BOOLEAN);
    assert_true(scalar_boolean_is_false(four_1));
    assert_false(scalar_boolean_is_true(four_1));

    reset_errno();
    node *five = mapping_get(root, (uint8_t *)"five", 4ul);
    assert_noerr();
    assert_not_null(five);
    assert_node_kind(five, SEQUENCE);
    node *five_0 = sequence_get(five, 0);
    assert_noerr();
    assert_node_kind(five_0, SCALAR);
    assert_scalar_value(five_0, "1.5");
    assert_scalar_kind(five_0, SCALAR_REAL);
    reset_errno();
    node *five_1 = sequence_get(five, 1);
    assert_noerr();
    assert_node_kind(five_1, SCALAR);
    assert_scalar_value(five_1, "42");
    assert_scalar_kind(five_1, SCALAR_INTEGER);
    reset_errno();
    node *five_2 = sequence_get(five, 2);
    assert_noerr();
    assert_node_kind(five_2, SCALAR);
    assert_scalar_value(five_2, "1978-07-26 10:15");
    assert_scalar_kind(five_2, SCALAR_TIMESTAMP);
    reset_errno();
}

void tag_setup(void)
{
    size_t yaml_size = strlen((char *)TAGGED_YAML);

    loader_context *loader = make_string_loader(TAGGED_YAML, yaml_size);
    assert_not_null(loader);
    tagged_model = load(loader);
    assert_not_null(tagged_model);
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));

    reset_errno();
    tagged_mapping_root = model_document_root(tagged_model, 0);
    assert_noerr();
    assert_not_null(tagged_mapping_root);
    
    assert_node_kind(tagged_mapping_root, MAPPING);
    assert_node_size(tagged_mapping_root, 7);

    loader_free(loader);
}

void tag_teardown(void)
{
    model_free(tagged_model);
    tagged_model = NULL;
    tagged_mapping_root = NULL;
}

START_TEST (shorthand_tags)
{
    assert_node_tag(tagged_mapping_root, "tag:vampire-squid.com,2008:instrument");

    reset_errno();
    node *asset_class = mapping_get(tagged_mapping_root, (uint8_t *)"asset-class", 11ul);
    assert_noerr();
    assert_not_null(asset_class);
    assert_node_kind(asset_class, SCALAR);
    assert_scalar_kind(asset_class, SCALAR_STRING);
    assert_node_tag(asset_class, "tag:vampire-squid.com,2008:asset-class");
    
    reset_errno();
    node *type = mapping_get(tagged_mapping_root, (uint8_t *)"type", 4ul);
    assert_noerr();
    assert_not_null(type);
    assert_node_kind(type, SCALAR);
    assert_scalar_kind(type, SCALAR_STRING);
    assert_node_tag(type, "tag:vampire-squid.com,2008:instrument/type");
    
    reset_errno();
    node *symbol = mapping_get(tagged_mapping_root, (uint8_t *)"symbol", 6ul);
    assert_noerr();
    assert_not_null(symbol);
    assert_node_kind(symbol, SCALAR);
    assert_scalar_kind(symbol, SCALAR_STRING);
    assert_node_tag(symbol, "tag:vampire-squid.com,2008:instrument/symbol");
}
END_TEST

START_TEST (explicit_tags)
{
    reset_errno();
    node *name = mapping_get(tagged_mapping_root, (uint8_t *)"name", 4ul);
    assert_noerr();
    assert_not_null(name);
    assert_node_kind(name, SCALAR);
    assert_scalar_kind(name, SCALAR_STRING);
    assert_node_tag(name, "tag:yaml.org,2002:str");
    
    reset_errno();
    node *exchange_rate = mapping_get(tagged_mapping_root, (uint8_t *)"exchange-rate", 13ul);
    assert_noerr();
    assert_not_null(exchange_rate);
    assert_node_kind(exchange_rate, SCALAR);
    assert_scalar_kind(exchange_rate, SCALAR_REAL);
    assert_node_tag(exchange_rate, "tag:yaml.org,2002:float");
    
    reset_errno();
    node *spot_date = mapping_get(tagged_mapping_root, (uint8_t *)"spot-date", 9ul);
    assert_noerr();
    assert_not_null(spot_date);
    assert_node_kind(spot_date, SCALAR);
    assert_scalar_kind(spot_date, SCALAR_TIMESTAMP);
    assert_node_tag(spot_date, "tag:yaml.org,2002:timestamp");
    
    reset_errno();
    node *settlement_date = mapping_get(tagged_mapping_root, (uint8_t *)"settlement-date", 15ul);
    assert_noerr();
    assert_not_null(settlement_date);
    assert_node_kind(settlement_date, SCALAR);
    assert_scalar_kind(settlement_date, SCALAR_TIMESTAMP);
    assert_node_tag(settlement_date, "tag:yaml.org,2002:timestamp");
}
END_TEST

document_model *anchor_setup(const unsigned char *yaml)
{
    loader_context *loader = make_string_loader(yaml, strlen((const char *)yaml));
    assert_not_null(loader);
    document_model *result = load(loader);
    assert_not_null(result);
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));

    reset_errno();
    node *root = model_document_root(result, 0);
    assert_noerr();
    assert_not_null(root);
    
    assert_node_kind(root, MAPPING);

    loader_free(loader);
    return result;
}

START_TEST (anchor)
{
    document_model *model = anchor_setup(ANCHOR_YAML);
    node *root = model_document_root(model, 0);
    
    reset_errno();
    node *one = mapping_get(root, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);

    reset_errno();
    node *alias = mapping_get(root, (uint8_t *)"two", 3ul);
    assert_noerr();
    assert_not_null(alias);
    assert_node_kind(alias, ALIAS);

    node *two = alias_target(alias);
    assert_not_null(two);
    assert_node_kind(two, SCALAR);
    assert_scalar_kind(two, SCALAR_STRING);
    assert_scalar_value(two, "bar1");

    model_free(model);
}
END_TEST

START_TEST (key_anchor)
{
    document_model *model = anchor_setup(KEY_ANCHOR_YAML);
    node *root = model_document_root(model, 0);

    reset_errno();
    node *one = mapping_get(root, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);

    reset_errno();
    node *alias1 = sequence_get(one, 1);
    assert_noerr();
    assert_not_null(alias1);
    assert_node_kind(alias1, ALIAS);

    node *one_1 = alias_target(alias1);
    assert_noerr();
    assert_not_null(one_1);
    assert_node_kind(one_1, SCALAR);
    assert_scalar_kind(one_1, SCALAR_STRING);
    assert_scalar_value(one_1, "one");
    reset_errno();

    node *alias2 = mapping_get(root, (uint8_t *)"two", 3ul);
    assert_noerr();
    assert_not_null(alias2);

    node *two = alias_target(alias2);
    assert_node_kind(two, SCALAR);
    assert_scalar_kind(two, SCALAR_STRING);
    assert_scalar_value(two, "one");
}
END_TEST

document_model *duplicate_setup(enum loader_duplicate_key_strategy value)
{
    size_t yaml_size = strlen((char *)DUPLICATE_KEY_YAML);

    loader_context *loader = make_string_loader(DUPLICATE_KEY_YAML, yaml_size);
    assert_not_null(loader);
    loader_set_dupe_strategy(loader, value);

    document_model *result = load(loader);
    assert_not_null(result);
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));

    reset_errno();
    node *root = model_document_root(result, 0);
    assert_noerr();
    assert_not_null(root);
    
    assert_node_kind(root, MAPPING);
    loader_free(loader);

    return result;
}

START_TEST (duplicate_clobber)
{
    document_model *model = duplicate_setup(DUPE_CLOBBER);
    node *root = model_document_root(model, 0);
    node *one = mapping_get(root, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SCALAR);
    assert_scalar_value(one, "bar");
    assert_scalar_kind(one, SCALAR_STRING);

    model_free(model);
}
END_TEST

START_TEST (duplicate_warn)
{
    document_model *model = duplicate_setup(DUPE_WARN);
    node *root = model_document_root(model, 0);
    node *one = mapping_get(root, (uint8_t *)"one", 3ul);
    assert_noerr();
    assert_not_null(one);
    assert_node_kind(one, SCALAR);
    assert_scalar_value(one, "bar");
    assert_scalar_kind(one, SCALAR_STRING);

    model_free(model);
}
END_TEST

START_TEST (duplicate_fail)
{
    size_t yaml_size = strlen((char *)DUPLICATE_KEY_YAML);

    loader_context *loader = make_string_loader(DUPLICATE_KEY_YAML, yaml_size);
    assert_not_null(loader);
    loader_set_dupe_strategy(loader, DUPE_FAIL);

    document_model *model = load(loader);
    assert_loader_failure(loader, ERR_DUPLICATE_KEY);

    model_free(model);
    loader_free(loader);
}
END_TEST

Suite *loader_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_string_input);
    tcase_add_test(bad_input_case, zero_string_input_length);
    tcase_add_test(bad_input_case, null_file_input);
    tcase_add_test(bad_input_case, eof_file_input);
    tcase_add_test(bad_input_case, null_context);
    tcase_add_test(bad_input_case, null_context_parser);
    tcase_add_test(bad_input_case, non_scalar_key);
    tcase_add_test(bad_input_case, alias_loop);
    tcase_add_test(bad_input_case, missing_anchor);

    TCase *file_case = tcase_create("file");
    tcase_add_test(file_case, load_from_file);

    TCase *string_case = tcase_create("string");
    tcase_add_test(string_case, load_from_string);

    TCase *tag_case = tcase_create("tag");
    tcase_add_unchecked_fixture(tag_case, tag_setup, tag_teardown);
    tcase_add_test(tag_case, shorthand_tags);
    tcase_add_test(tag_case, explicit_tags);

    TCase *anchor_case = tcase_create("anchor");
    tcase_add_test(anchor_case, anchor);
    tcase_add_test(anchor_case, key_anchor);

    TCase *duplicate_case = tcase_create("duplicate");
    tcase_add_test(duplicate_case, duplicate_clobber);
    tcase_add_test(duplicate_case, duplicate_warn);
    tcase_add_test(duplicate_case, duplicate_fail);

    Suite *loader = suite_create("Loader");
    suite_add_tcase(loader, bad_input_case);
    suite_add_tcase(loader, file_case);
    suite_add_tcase(loader, string_case);
    suite_add_tcase(loader, tag_case);
    suite_add_tcase(loader, anchor_case);
    suite_add_tcase(loader, duplicate_case);

    return loader;
}
