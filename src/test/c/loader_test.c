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
    "five: 1.5\n";

static void assert_model_state(loader_context *loader, document_model *model);

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
    assert_node_kind(five, SCALAR);
    assert_scalar_value(five, "1.5");
    assert_scalar_kind(five, SCALAR_NUMBER);
}

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

    Suite *loader = suite_create("Loader");
    suite_add_tcase(loader, bad_input_case);
    suite_add_tcase(loader, file_case);
    suite_add_tcase(loader, string_case);
    
    return loader;
}

