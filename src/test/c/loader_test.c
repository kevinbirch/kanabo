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

static document_model *tagged_model = NULL;
static node *tagged_mapping_root = NULL;

static void assert_model_state(loader_context *loader, document_model *model);
void tag_setup(void);
void tag_teardown(void);

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
    document_model *model = make_model(1);
    loader_context *loader = (loader_context *)calloc(1, sizeof(loader_context));
    loader->parser = NULL;
    loader->model = model;

    reset_errno();
    assert_null(load(loader));
    assert_errno(EINVAL);    

    model_free(model);
    free(loader);
}
END_TEST

START_TEST (null_context_model)
{
    yaml_parser_t parser;
    loader_context *loader = (loader_context *)calloc(1, sizeof(loader_context));
    loader->parser = &parser;
    loader->model = NULL;

    reset_errno();
    assert_null(load(loader));
    assert_errno(EINVAL);    

    free(loader);
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
    assert_not_null(mapping_get_all(root));

    reset_errno();
    node *one = mapping_get(root, "one");
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
    node *two = mapping_get(root, "two");
    assert_noerr();
    assert_not_null(two);
    assert_node_kind(two, SCALAR);
    assert_scalar_value(two, "foo2");
    assert_scalar_kind(two, SCALAR_STRING);

    reset_errno();
    node *three = mapping_get(root, "three");
    assert_noerr();
    assert_not_null(three);
    assert_node_kind(three, SCALAR);
    assert_scalar_value(three, "null");
    assert_scalar_kind(three, SCALAR_NULL);

    reset_errno();
    node *four = mapping_get(root, "four");
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
    node *five = mapping_get(root, "five");
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
    assert_not_null(mapping_get_all(tagged_mapping_root));

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
    node *asset_class = mapping_get(tagged_mapping_root, "asset-class");
    assert_noerr();
    assert_not_null(asset_class);
    assert_node_kind(asset_class, SCALAR);
    assert_scalar_kind(asset_class, SCALAR_STRING);
    assert_node_tag(asset_class, "tag:vampire-squid.com,2008:asset-class");
    
    reset_errno();
    node *type = mapping_get(tagged_mapping_root, "type");
    assert_noerr();
    assert_not_null(type);
    assert_node_kind(type, SCALAR);
    assert_scalar_kind(type, SCALAR_STRING);
    assert_node_tag(type, "tag:vampire-squid.com,2008:instrument/type");
    
    reset_errno();
    node *symbol = mapping_get(tagged_mapping_root, "symbol");
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
    node *name = mapping_get(tagged_mapping_root, "name");
    assert_noerr();
    assert_not_null(name);
    assert_node_kind(name, SCALAR);
    assert_scalar_kind(name, SCALAR_STRING);
    assert_node_tag(name, "tag:yaml.org,2002:str");
    
    reset_errno();
    node *exchange_rate = mapping_get(tagged_mapping_root, "exchange-rate");
    assert_noerr();
    assert_not_null(exchange_rate);
    assert_node_kind(exchange_rate, SCALAR);
    assert_scalar_kind(exchange_rate, SCALAR_REAL);
    assert_node_tag(exchange_rate, "tag:yaml.org,2002:float");
    
    reset_errno();
    node *spot_date = mapping_get(tagged_mapping_root, "spot-date");
    assert_noerr();
    assert_not_null(spot_date);
    assert_node_kind(spot_date, SCALAR);
    assert_scalar_kind(spot_date, SCALAR_TIMESTAMP);
    assert_node_tag(spot_date, "tag:yaml.org,2002:timestamp");
    
    reset_errno();
    node *settlement_date = mapping_get(tagged_mapping_root, "settlement-date");
    assert_noerr();
    assert_not_null(settlement_date);
    assert_node_kind(settlement_date, SCALAR);
    assert_scalar_kind(settlement_date, SCALAR_TIMESTAMP);
    assert_node_tag(settlement_date, "tag:yaml.org,2002:timestamp");
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
    tcase_add_test(bad_input_case, null_context_model);

    TCase *file_case = tcase_create("file");
    tcase_add_test(file_case, load_from_file);

    TCase *string_case = tcase_create("string");
    tcase_add_test(string_case, load_from_string);

    TCase *tag_case = tcase_create("tag");
    tcase_add_unchecked_fixture(tag_case, tag_setup, tag_teardown);
    tcase_add_test(tag_case, shorthand_tags);
    tcase_add_test(tag_case, explicit_tags);

    Suite *loader = suite_create("Loader");
    suite_add_tcase(loader, bad_input_case);
    suite_add_tcase(loader, file_case);
    suite_add_tcase(loader, string_case);
    suite_add_tcase(loader, tag_case);

    return loader;
}

