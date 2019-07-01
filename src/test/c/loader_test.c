#include <stdio.h>
#include <string.h>

#include "test.h"
#include "loader.h"
#include "test_document.h"

static DocumentSet *model_fixture = NULL;

#define assert_loader_failure(CONTEXT, EXPECTED_RESULT)                 \
    do                                                                  \
    {                                                                   \
        assert_nothing((CONTEXT));                                      \
        assert_uint_eq(1, vector_length((CONTEXT).error));              \
        LoaderError *e = vector_last((CONTEXT).error);                  \
        assert_uint_eq((EXPECTED_RESULT), e->code);                     \
    } while(0)

static inline void dispose_maybe(Maybe(DocumentSet) maybe)
{
    if(is_nothing(maybe))
    {
        loader_dispose_errors(from_nothing(maybe));
    }
    else
    {
        dispose_document_set(from_just(maybe));
    }
}

static Maybe(DocumentSet) load(const char *filename, DuplicateKeyStrategy strategy)
{
    assert_not_null(filename);
    Maybe(Input) input = make_input_from_file(filename);
    if(is_nothing(input))
    {
        InputError err = from_nothing(input);
        printf("%s: %s: %s\n", filename, input_strerror(err.code), strerror(err.errno_val));
    }
    assert_just(input);

    Maybe(DocumentSet) documents = load_yaml(from_just(input), strategy);
    
    dispose_input(from_just(input));

    return documents;
}

static DocumentSet *must_load(const char *filename, DuplicateKeyStrategy strategy)
{
    assert_not_null(filename);
    Maybe(Input) input = make_input_from_file(filename);
    assert_just(input);

    Maybe(DocumentSet) yaml = load_yaml(from_just(input), strategy);
    assert_just(yaml);

    DocumentSet *documents = from_just(yaml);
    assert_uint_eq(1, document_set_size(documents));

    dispose_input(from_just(input));

    return documents;
}

static void tagged_yaml_setup(void)
{
    model_fixture = must_load("test-resources/tagged.yaml", DUPE_FAIL);

    Document *document = document_set_get(model_fixture, 0);
    assert_uint_eq(1, vector_length(document->yaml.tags));
    assert_int_eq(1, document->yaml.major);
    assert_int_eq(1, document->yaml.minor);

    Node *root = document_set_get_root(model_fixture, 0);
    assert_not_null(root);

    assert_node_kind(root, MAPPING);
    assert_node_size(root, 7);
}

static void anchor_yaml_setup(void)
{
    model_fixture = must_load("test-resources/anchor.yaml", DUPE_FAIL);

    Node *root = document_set_get_root(model_fixture, 0);
    assert_not_null(root);

    assert_node_kind(root, MAPPING);
    assert_node_size(root, 2);
}

static void key_anchor_yaml_setup(void)
{
    model_fixture = must_load("test-resources/key-anchor.yaml", DUPE_FAIL);

    Node *root = document_set_get_root(model_fixture, 0);
    assert_not_null(root);

    assert_node_kind(root, MAPPING);
    assert_node_size(root, 2);
}

static void loader_teardown(void)
{
    dispose_document_set(model_fixture);
    model_fixture = NULL;
}

START_TEST (alias_loop)
{
    Maybe(DocumentSet) documents = load("test-resources/alias-loop.yaml", DUPE_FAIL);
    assert_loader_failure(documents, ERR_ALIAS_LOOP);
    dispose_maybe(documents);
}
END_TEST

START_TEST (non_scalar_key)
{
    Maybe(DocumentSet) documents = load("test-resources/non-scalar-key.yaml", DUPE_FAIL);
    assert_loader_failure(documents, ERR_NON_SCALAR_KEY);
    dispose_maybe(documents);
}
END_TEST

START_TEST (missing_anchor)
{
    Maybe(DocumentSet) documents = load("test-resources/missing-anchor.yaml", DUPE_FAIL);
    assert_loader_failure(documents, ERR_NO_ANCHOR_FOR_ALIAS);
    dispose_maybe(documents);
}
END_TEST

static void assert_model_state(DocumentSet *model)
{
    assert_not_null(model);
    assert_uint_eq(1, document_set_size(model));

    Node *root = document_set_get_root(model, 0);
    assert_not_null(root);

    assert_node_kind(root, MAPPING);
    assert_node_size(root, 5);

    String *key = NULL;

    key = make_string("one");
    Node *one = mapping_lookup(mapping(root), key);
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);
    dispose_string(key);

    Node *one_0 = sequence_get(sequence(one), 0);
    assert_node_kind(one_0, SCALAR);
    assert_scalar_value((one_0), "foo1");
    assert_scalar_kind(one_0, SCALAR_STRING);
    Node *one_1 = sequence_get(sequence(one), 1);
    assert_node_kind(one_1, SCALAR);
    assert_scalar_value((one_1), "bar1");
    assert_scalar_kind(one_1, SCALAR_STRING);

    key = make_string("two");
    Node *two = mapping_lookup(mapping(root), key);
    assert_not_null(two);
    assert_node_kind(two, SCALAR);
    assert_scalar_value((two), "foo2");
    assert_scalar_kind(two, SCALAR_STRING);
    dispose_string(key);

    key = make_string("three");
    Node *three = mapping_lookup(mapping(root), key);
    assert_not_null(three);
    assert_node_kind(three, SCALAR);
    assert_scalar_value((three), "null");
    assert_scalar_kind(three, SCALAR_NULL);
    dispose_string(key);

    key = make_string("four");
    Node *four = mapping_lookup(mapping(root), key);
    assert_not_null(four);
    assert_node_kind(four, SEQUENCE);
    dispose_string(key);

    Node *four_0 = sequence_get(sequence(four), 0);
    assert_node_kind(four_0, SCALAR);
    assert_scalar_value((four_0), "true");
    assert_scalar_kind(four_0, SCALAR_BOOLEAN);
    assert_true(scalar_boolean_is_true(scalar(four_0)));
    assert_false(scalar_boolean_is_false(scalar(four_0)));
    assert_true(scalar(four_0)->boolean);
    Node *four_1 = sequence_get(sequence(four), 1);
    assert_node_kind(four_0, SCALAR);
    assert_scalar_value((four_1), "false");
    assert_scalar_kind(four_1, SCALAR_BOOLEAN);
    assert_true(scalar_boolean_is_false(scalar(four_1)));
    assert_false(scalar_boolean_is_true(scalar(four_1)));
    assert_false(scalar(four_1)->boolean);

    key = make_string("five");
    Node *five = mapping_lookup(mapping(root), key);
    assert_not_null(five);
    assert_node_kind(five, SEQUENCE);
    Node *five_0 = sequence_get(sequence(five), 0);
    assert_node_kind(five_0, SCALAR);
    assert_scalar_value((five_0), "1.5");
    assert_scalar_kind(five_0, SCALAR_REAL);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    assert_true(1.5 == scalar(five_0)->real);
#pragma GCC diagnostic pop
    Node *five_1 = sequence_get(sequence(five), 1);
    assert_node_kind(five_1, SCALAR);
    assert_scalar_value((five_1), "42");
    assert_scalar_kind(five_1, SCALAR_INTEGER);
    assert_int_eq(42, scalar(five_1)->integer);
    Node *five_2 = sequence_get(sequence(five), 2);
    assert_node_kind(five_2, SCALAR);
    assert_scalar_value(five_2, "1978-07-26 10:15:00");
    assert_scalar_kind(five_2, SCALAR_TIMESTAMP);
    dispose_string(key);
}

START_TEST (load_from_file)
{
    DocumentSet *documents = must_load("test-resources/loader-fixture.yaml", DUPE_FAIL);
    assert_model_state(documents);

    dispose_document_set(documents);
}
END_TEST

START_TEST (shorthand_tags)
{
    Mapping *root = mapping(document_set_get_root(model_fixture, 0));
    assert_node_tag(node(root), "tag:vampire-squid.com,2008:instrument");

    String *asset_class = make_string("asset-class");
    Node *asset_class_value = mapping_lookup(root, asset_class);
    assert_not_null(asset_class_value);
    assert_node_kind(asset_class_value, SCALAR);
    assert_scalar_kind(asset_class_value, SCALAR_STRING);
    assert_node_tag(asset_class_value, "tag:vampire-squid.com,2008:asset-class");
    dispose_string(asset_class);

    String *type = make_string("type");
    Node *type_value = mapping_lookup(root, type);
    assert_not_null(type_value);
    assert_node_kind(type_value, SCALAR);
    assert_scalar_kind(type_value, SCALAR_STRING);
    assert_node_tag(type_value, "tag:vampire-squid.com,2008:instrument/type");
    dispose_string(type);

    String *symbol = make_string("symbol");
    Node *symbol_value = mapping_lookup(root, symbol);
    assert_not_null(symbol_value);
    assert_node_kind(symbol_value, SCALAR);
    assert_scalar_kind(symbol_value, SCALAR_STRING);
    assert_node_tag(symbol_value, "tag:vampire-squid.com,2008:instrument/symbol");
    dispose_string(symbol);
}
END_TEST

START_TEST (explicit_tags)
{
    Mapping *root = mapping(document_set_get_root(model_fixture, 0));

    String *name = make_string("name");
    Node *name_value = mapping_lookup(root, name);
    assert_not_null(name_value);
    assert_node_kind(name_value, SCALAR);
    assert_scalar_kind(name_value, SCALAR_STRING);
    assert_node_tag(name_value, "tag:yaml.org,2002:str");
    dispose_string(name);

    String *exchange_rate = make_string("exchange-rate");
    Node *exchange_rate_value = mapping_lookup(root, exchange_rate);
    assert_not_null(exchange_rate_value);
    assert_node_kind(exchange_rate_value, SCALAR);
    assert_scalar_kind(exchange_rate_value, SCALAR_REAL);
    assert_node_tag(exchange_rate_value, "tag:yaml.org,2002:float");
    dispose_string(exchange_rate);

    String *spot_date = make_string("spot-date");
    Node *spot_date_value = mapping_lookup(root, spot_date);
    assert_not_null(spot_date_value);
    assert_node_kind(spot_date_value, SCALAR);
    assert_scalar_kind(spot_date_value, SCALAR_TIMESTAMP);
    assert_node_tag(spot_date_value, "tag:yaml.org,2002:timestamp");
    dispose_string(spot_date);

    String *settlement_date = make_string("settlement-date");
    Node *settlement_date_value = mapping_lookup(root, settlement_date);
    assert_not_null(settlement_date_value);
    assert_node_kind(settlement_date_value, SCALAR);
    assert_scalar_kind(settlement_date_value, SCALAR_TIMESTAMP);
    assert_node_tag(settlement_date_value, "tag:yaml.org,2002:timestamp");
    dispose_string(settlement_date);
}
END_TEST

START_TEST (anchor)
{
    Mapping *root = mapping(document_set_get_root(model_fixture, 0));

    String *key = NULL;

    key = make_string("one");
    Node *one = mapping_lookup(root, key);
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);
    dispose_string(key);

    key = make_string("two");
    Node *a = mapping_lookup(root, key);
    assert_not_null(a);
    assert_node_kind(a, ALIAS);
    dispose_string(key);

    Node *two = alias_target(alias(a));
    assert_not_null(two);
    assert_node_kind(two, SCALAR);
    assert_scalar_kind(two, SCALAR_STRING);
    assert_scalar_value(two, "bar1");
}
END_TEST

START_TEST (key_anchor)
{
    Mapping *root = mapping(document_set_get_root(model_fixture, 0));

    String *key = NULL;

    key = make_string("one");
    Node *one = mapping_lookup(root, key);
    assert_not_null(one);
    assert_node_kind(one, SEQUENCE);
    assert_node_size(one, 2);
    dispose_string(key);

    Node *alias1 = sequence_get(sequence(one), 1);
    assert_not_null(alias1);
    assert_node_kind(alias1, ALIAS);

    Node *one_1 = alias_target(alias(alias1));
    assert_not_null(one_1);
    assert_node_kind(one_1, SCALAR);
    assert_scalar_kind(one_1, SCALAR_STRING);
    assert_scalar_value(one_1, "one");

    key = make_string("two");
    Node *alias2 = mapping_lookup(root, key);
    assert_not_null(alias2);
    dispose_string(key);

    Node *two = alias_target(alias(alias2));
    assert_node_kind(two, SCALAR);
    assert_scalar_kind(two, SCALAR_STRING);
    assert_scalar_value(two, "one");
}
END_TEST

START_TEST (duplicate_clobber)
{
    DocumentSet *documents = must_load("test-resources/duplicate-key.yaml", DUPE_CLOBBER);

    Node *root = document_set_get_root(documents, 0);
    assert_node_kind(root, MAPPING);
    assert_node_size(root, 3);

    String *key = make_string("one");
    Node *one = mapping_lookup(mapping(root), key);
    assert_not_null(one);
    assert_node_kind(one, SCALAR);
    assert_scalar_value(one, "bar2");
    assert_scalar_kind(one, SCALAR_STRING);

    dispose_string(key);
    dispose_document_set(documents);
}
END_TEST

START_TEST (duplicate_fail)
{
    Maybe(DocumentSet) documents = load("test-resources/duplicate-key.yaml", DUPE_FAIL);
    assert_loader_failure(documents, ERR_DUPLICATE_KEY);

    dispose_maybe(documents);
}
END_TEST

Suite *loader_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, alias_loop);
    tcase_add_test(bad_input_case, non_scalar_key);
    tcase_add_test(bad_input_case, missing_anchor);

    TCase *file_case = tcase_create("file");
    tcase_add_test(file_case, load_from_file);

    TCase *tag_case = tcase_create("tag");
    tcase_add_unchecked_fixture(tag_case, tagged_yaml_setup, loader_teardown);
    tcase_add_test(tag_case, shorthand_tags);
    tcase_add_test(tag_case, explicit_tags);

    TCase *anchor_case = tcase_create("anchor");
    tcase_add_unchecked_fixture(anchor_case, anchor_yaml_setup, loader_teardown);
    tcase_add_test(anchor_case, anchor);

    TCase *key_anchor_case = tcase_create("key_anchor");
    tcase_add_unchecked_fixture(key_anchor_case, key_anchor_yaml_setup, loader_teardown);
    tcase_add_test(key_anchor_case, key_anchor);

    TCase *duplicate_case = tcase_create("duplicate");
    tcase_add_test(duplicate_case, duplicate_clobber);
    tcase_add_test(duplicate_case, duplicate_fail);

    Suite *loader = suite_create("Loader");
    suite_add_tcase(loader, bad_input_case);
    suite_add_tcase(loader, file_case);
    suite_add_tcase(loader, tag_case);
    suite_add_tcase(loader, anchor_case);
    suite_add_tcase(loader, key_anchor_case);
    suite_add_tcase(loader, duplicate_case);

    return loader;
}
