#include "test.h"
#include "test_document.h"
#include "test_nodelist.h"

// check defines a fail helper that conflicts with the maybe constructor
#undef fail

#include "evaluator.h"
#include "evaluator/private.h"
#undef component_name
#include "parser.h"
#include "loader.h"

void inventory_setup(void);
void invoice_setup(void);
document_model *load_file(char *name);

void evaluator_teardown(void);

bool scalar_true(node *each, void *context);

static document_model *model = NULL;

#define assert_evaluator_failure(CONTEXT, EXPECTED_RESULT)              \
    do                                                                  \
    {                                                                   \
        assert_not_null((CONTEXT));                                     \
        assert_int_eq((EXPECTED_RESULT), evaluator_status((CONTEXT)));  \
        log_debug("evaluator test", "received expected failure message: '%s'", evaluator_status_message((CONTEXT))); \
    } while(0)


START_TEST (null_model)
{
    char *expression = "foo";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(NULL, &from_just(maybe));
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_MODEL_IS_NULL);
    dispose_path(from_just(maybe));
    evaluator_free(evaluator);
}
END_TEST

START_TEST (null_path)
{
    document_model *bad_model = make_model();
    evaluator_context *evaluator = make_evaluator(bad_model, NULL);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_PATH_IS_NULL);
    model_free(bad_model);
    evaluator_free(evaluator);
}
END_TEST

START_TEST (null_document)
{
    char *expression = "foo";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    document_model *bad_model = make_model();
    evaluator_context *evaluator = make_evaluator(bad_model, &from_just(maybe));
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_NO_DOCUMENT_IN_MODEL);
    model_free(bad_model);
    dispose_path(from_just(maybe));
    evaluator_free(evaluator);
}
END_TEST

START_TEST (null_document_root)
{
    char *expression = "foo";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    document_model *bad_model = make_model();
    node *document = (node *)calloc(1, sizeof(struct node));
    assert_not_null(document);
    document->tag.kind = DOCUMENT;
    document->tag.name = NULL;
    document->content.size = 0;
    document->content.target = NULL;
    model_add(bad_model, document);

    evaluator_context *evaluator = make_evaluator(bad_model, &from_just(maybe));
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_NO_ROOT_IN_DOCUMENT);
    model_free(bad_model);
    dispose_path(from_just(maybe));
    evaluator_free(evaluator);
}
END_TEST

START_TEST (relative_path)
{
    char *expression = "foo";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    document_model *bad_model = make_model();
    node *root = make_mapping_node();
    node *document = make_document_node();
    document_set_root(document, root);
    model_add(bad_model, document);

    evaluator_context *evaluator = make_evaluator(bad_model, &from_just(maybe));
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_PATH_IS_NOT_ABSOLUTE);
    model_free(bad_model);
    dispose_path(from_just(maybe));
    evaluator_free(evaluator);
}
END_TEST


START_TEST (empty_path)
{
    JsonPath *path = (JsonPath *)calloc(1, sizeof(JsonPath));
    assert_not_null(path);
    path->kind = ABSOLUTE_PATH;
    path->steps = NULL;

    document_model *bad_model = make_model();
    node *root = make_mapping_node();
    node *document = make_document_node();
    document_set_root(document, root);
    model_add(bad_model, document);

    evaluator_context *evaluator = make_evaluator(bad_model, path);
    assert_not_null(evaluator);
    assert_errno(EINVAL);

    assert_evaluator_failure(evaluator, ERR_PATH_IS_EMPTY);
    model_free(bad_model);
    free(path);
    evaluator_free(evaluator);
}
END_TEST

START_TEST (null_context)
{
    reset_errno();
    assert_null(evaluate(NULL));
    assert_errno(EINVAL);    
}
END_TEST

START_TEST (null_context_model)
{
    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = NULL;
    evaluator->path = NULL;
    evaluator->list = NULL;

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    free(evaluator);
}
END_TEST

START_TEST (null_context_path)
{
    document_model *bad_model = make_model();
    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = NULL;
    evaluator->list = NULL;

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    free(evaluator);
}
END_TEST

START_TEST (null_context_list)
{
    document_model *bad_model = make_model();
    JsonPath *path = (JsonPath *)calloc(1, sizeof(JsonPath));

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = path;
    evaluator->list = NULL;

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    free(path);
    free(evaluator);
}
END_TEST

START_TEST (null_context_document)
{
    char *expression = "foo";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    document_model *bad_model = make_model();
    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = &from_just(maybe);
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    dispose_path(from_just(maybe));
    free(evaluator);
}
END_TEST

START_TEST (null_context_document_root)
{
    char *expression = "foo";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    node *document = (node *)calloc(1, sizeof(struct node));
    assert_not_null(document);
    document->tag.kind = DOCUMENT;
    document->tag.name = NULL;
    document->content.size = 0;
    document->content.target = NULL;

    document_model *bad_model = make_model();
    model_add(bad_model, document);

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = &from_just(maybe);
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    nodelist_free(evaluator->list);
    dispose_path(from_just(maybe));
    free(evaluator);
}
END_TEST

START_TEST (relative_context_path)
{
    char *expression = "foo";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    document_model *bad_model = make_model();
    node *root = make_mapping_node();
    node *document = make_document_node();
    document_set_root(document, root);
    model_add(bad_model, document);

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = &from_just(maybe);
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    nodelist_free(evaluator->list);
    dispose_path(from_just(maybe));
    free(evaluator);
}
END_TEST

START_TEST (empty_context_path)
{
    JsonPath *path = (JsonPath *)calloc(1, sizeof(JsonPath));
    assert_not_null(path);
    path->kind = ABSOLUTE_PATH;
    path->steps = NULL;

    document_model *bad_model = make_model();
    node *root = make_mapping_node();
    node *document = make_document_node();
    document_set_root(document, root);
    model_add(bad_model, document);

    evaluator_context *evaluator = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    evaluator->model = bad_model;
    evaluator->path = path;
    evaluator->list = make_nodelist();

    reset_errno();
    assert_null(evaluate(evaluator));
    assert_errno(EINVAL);

    model_free(bad_model);
    nodelist_free(evaluator->list);
    free(path);
    free(evaluator);
}
END_TEST

void inventory_setup(void)
{
    model = load_file("test-resources/inventory.json");
}

void invoice_setup(void)
{
    model = load_file("test-resources/invoice.yaml");
}

document_model *load_file(char *name)
{
    FILE *input = fopen(name, "r");
    assert_not_null(input);
    
    loader_context *loader = make_file_loader(input);    
    assert_not_null(loader);
    document_model *result = load(loader);
    assert_not_null(result);
    assert_int_eq(LOADER_SUCCESS, loader_status(loader));

    int closed = fclose(input);
    assert_int_eq(0, closed);
    loader_free(loader);

    return result;
}

void evaluator_teardown(void)
{
    model_free(model);
    model = NULL;
}

START_TEST (dollar_only)
{
    char *expression = "$";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);
    
    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);
    assert_node_kind(nodelist_get(list, 0), MAPPING);
    assert_node_size(nodelist_get(list, 0), 1);
    assert_mapping_has_key(nodelist_get(list, 0), "store");

    nodelist_free(list);
}
END_TEST

START_TEST (single_name_step)
{
    char *expression = "$.store";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);
    node *store = nodelist_get(list, 0);
    assert_not_null(store);

    assert_node_kind(store, MAPPING);
    assert_node_size(store, 2);
    assert_mapping_has_key(store, "book");
    assert_mapping_has_key(store, "bicycle");

    nodelist_free(list);
}
END_TEST

START_TEST (simple_recursive_step)
{
    char *expression = "$..author";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 5);

    assert_node_kind(nodelist_get(list, 0), SCALAR);
    assert_node_kind(nodelist_get(list, 1), SCALAR);
    assert_node_kind(nodelist_get(list, 2), SCALAR);
    assert_node_kind(nodelist_get(list, 3), SCALAR);

    nodelist_free(list);
}
END_TEST

START_TEST (compound_recursive_step)
{
    char *expression = "$.store..price";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 6);

    assert_scalar_kind(nodelist_get(list, 0), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 1), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 2), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 3), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 4), SCALAR_REAL);
    assert_scalar_kind(nodelist_get(list, 5), SCALAR_REAL);
    
    nodelist_free(list);
}
END_TEST

START_TEST (long_path)
{
    char *expression = "$.store.bicycle.color";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);
    node *color = nodelist_get(list, 0);
    assert_not_null(color);

    assert_node_kind(color, SCALAR);
    assert_scalar_value(color, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard)
{
    char *expression = "$.store.*";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 6);

    assert_node_kind(nodelist_get(list, 0), MAPPING);
    assert_node_kind(nodelist_get(list, 1), MAPPING);
    assert_node_kind(nodelist_get(list, 2), MAPPING);
    assert_node_kind(nodelist_get(list, 3), MAPPING);
    assert_node_kind(nodelist_get(list, 4), MAPPING);

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_wildcard)
{
    char *expression = "$..*";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 34);

    assert_node_kind(nodelist_get(list, 0), MAPPING);

    nodelist_free(list);
}
END_TEST

START_TEST (object_test)
{
    char *expression = "$.store.object()";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    node *value = nodelist_get(list, 0);
    assert_not_null(value);

    assert_node_kind(value, MAPPING);
    assert_mapping_has_key(value, "book");
    assert_mapping_has_key(value, "bicycle");

    nodelist_free(list);
}
END_TEST

START_TEST (array_test)
{
    char *expression = "$.store.book.array()";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    node *value = nodelist_get(list, 0);
    assert_not_null(value);
    assert_node_kind(value, SEQUENCE);
    assert_node_size(value, 5);

    nodelist_free(list);
}
END_TEST

START_TEST (number_test)
{
    char *expression = "$.store.book[*].price.number()";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 5);

    assert_scalar_value(nodelist_get(list, 0), "8.95");
    assert_scalar_value(nodelist_get(list, 1), "12.99");
    assert_scalar_value(nodelist_get(list, 2), "8.99");
    assert_scalar_value(nodelist_get(list, 3), "22.99");
    
    nodelist_free(list);
}
END_TEST

bool scalar_true(node *each, void *context)
{

    return SCALAR == node_kind(each) &&
        SCALAR_BOOLEAN == scalar_kind(each) &&
        scalar_boolean_is_true(each);
}

START_TEST (wildcard_predicate)
{
    char *expression = "$.store.book[*].author";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 5);

    assert_node_kind(nodelist_get(list, 0), SCALAR);
    assert_node_kind(nodelist_get(list, 1), SCALAR);
    assert_node_kind(nodelist_get(list, 2), SCALAR);
    assert_node_kind(nodelist_get(list, 3), SCALAR);

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate_on_mapping)
{
    char *expression = "$.store.bicycle[*].color";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate_on_scalar)
{
    char *expression = "$.store.bicycle.color[*]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    node *scalar = nodelist_get(list, 0);
    assert_node_kind(scalar, SCALAR);
    assert_scalar_value(scalar, "red");

    nodelist_free(list);
}
END_TEST

START_TEST (subscript_predicate)
{
    char *expression = "$.store.book[2]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    reset_errno();
    node *mapping = nodelist_get(list, 0);
    assert_node_kind(mapping, MAPPING);
    assert_noerr();
    reset_errno();
    node *author = mapping_get(mapping, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Herman Melville");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_subscript_predicate)
{
    char *expression = "$..book[2]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    reset_errno();
    node *mapping = nodelist_get(list, 0);
    assert_node_kind(mapping, MAPPING);
    assert_noerr();
    reset_errno();
    node *author = mapping_get(mapping, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Herman Melville");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate)
{
    char *expression = "$.store.book[:2]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 2);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    reset_errno();
    book = nodelist_get(list, 1);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Evelyn Waugh");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_slice_predicate)
{
    char *expression = "$..book[:2]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 2);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    reset_errno();
    book = nodelist_get(list, 1);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Evelyn Waugh");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_with_step)
{
    char *expression = "$.store.book[:2:2]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "Nigel Rees");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_negative_from)
{
    char *expression = "$.store.book[-1:]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "夏目漱石 (NATSUME Sōseki)");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_slice_predicate_negative_from)
{
    char *expression = "$..book[-1:]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);

    reset_errno();
    node *book = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(book, MAPPING);

    reset_errno();
    node *author = mapping_get(book, (uint8_t *)"author", 6ul);
    assert_noerr();
    assert_not_null(author);
    assert_scalar_value(author, "夏目漱石 (NATSUME Sōseki)");

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_copy)
{
    char *expression = "$.store.book[::]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 5);

    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 0), (uint8_t *)"author", 6ul), "Nigel Rees");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 1), (uint8_t *)"author", 6ul), "Evelyn Waugh");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 2), (uint8_t *)"author", 6ul), "Herman Melville");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 3), (uint8_t *)"author", 6ul), "J. R. R. Tolkien");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (slice_predicate_reverse)
{
    char *expression = "$.store.book[::-1]";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 5);

    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 0), (uint8_t *)"author", 6ul), "夏目漱石 (NATSUME Sōseki)");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 1), (uint8_t *)"author", 6ul), "J. R. R. Tolkien");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 2), (uint8_t *)"author", 6ul), "Herman Melville");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 3), (uint8_t *)"author", 6ul), "Evelyn Waugh");
    assert_noerr();
    reset_errno();
    assert_scalar_value(mapping_get(nodelist_get(list, 4), (uint8_t *)"author", 6ul), "Nigel Rees");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (name_alias)
{
    char *expression = "$.payment.billing-address.name";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);
    reset_errno();
    assert_scalar_value(nodelist_get(list, 0), "Ramond Hessel");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (type_alias)
{
    char *expression = "$.shipments[0].*.number()";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 1);
    reset_errno();
    assert_scalar_value(nodelist_get(list, 0), "237.23");
    assert_noerr();

    nodelist_free(list);
}
END_TEST

START_TEST (greedy_wildcard_alias)
{
    char *expression = "$.shipments[0].items.*";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 2);

    reset_errno();
    node *zero = nodelist_get(list, 0);
    assert_noerr();
    assert_node_kind(zero, MAPPING);
    assert_scalar_value(mapping_get(zero, (uint8_t *)"isbn", 4), "1428312250");

    node *one = nodelist_get(list, 1);
    assert_noerr();
    assert_node_kind(one, MAPPING);
    assert_scalar_value(mapping_get(one, (uint8_t *)"isbn", 4), "0323073867");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_alias)
{
    char *expression = "$.shipments..isbn";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 2);

    assert_scalar_value(nodelist_get(list, 0), "1428312250");
    assert_scalar_value(nodelist_get(list, 1), "0323073867");

    nodelist_free(list);
}
END_TEST

START_TEST (wildcard_predicate_alias)
{
    char *expression = "$.shipments[0].items[*].price";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 2);

    assert_scalar_value(nodelist_get(list, 0), "135.48");
    assert_scalar_value(nodelist_get(list, 1), "84.18");

    nodelist_free(list);
}
END_TEST

START_TEST (recursive_wildcard_alias)
{
    char *expression = "$.shipments[0].items..*";

    Maybe(JsonPath) maybe = parse(expression);
    assert_int_eq(JUST, maybe.tag);

    evaluator_context *evaluator = make_evaluator(model, &from_just(maybe));
    assert_not_null(evaluator);

    nodelist *list = evaluate(evaluator);
    assert_int_eq(EVALUATOR_SUCCESS, evaluator_status(evaluator));
    assert_not_null(list);
    evaluator_free(evaluator);
    dispose_path(from_just(maybe));

    assert_nodelist_length(list, 15);

    nodelist_free(list);
}
END_TEST

Suite *evaluator_suite(void)
{
    TCase *bad_input_case = tcase_create("bad input");
    tcase_add_test(bad_input_case, null_model);
    tcase_add_test(bad_input_case, null_path);
    tcase_add_test(bad_input_case, null_document);
    tcase_add_test(bad_input_case, null_document_root);
    tcase_add_test(bad_input_case, relative_path);
    tcase_add_test(bad_input_case, empty_path);
    tcase_add_test(bad_input_case, null_context);
    tcase_add_test(bad_input_case, null_context_model);
    tcase_add_test(bad_input_case, null_context_path);
    tcase_add_test(bad_input_case, null_context_list);
    tcase_add_test(bad_input_case, null_context_document);
    tcase_add_test(bad_input_case, null_context_document_root);
    tcase_add_test(bad_input_case, relative_context_path);
    tcase_add_test(bad_input_case, empty_context_path);

    TCase *basic_case = tcase_create("basic");
    tcase_add_unchecked_fixture(basic_case, inventory_setup, evaluator_teardown);
    tcase_add_test(basic_case, dollar_only);
    tcase_add_test(basic_case, single_name_step);
    tcase_add_test(basic_case, long_path);
    tcase_add_test(basic_case, wildcard);
    tcase_add_test(basic_case, object_test);
    tcase_add_test(basic_case, array_test);
    tcase_add_test(basic_case, number_test);

    TCase *predicate_case = tcase_create("predicate");
    tcase_add_unchecked_fixture(predicate_case, inventory_setup, evaluator_teardown);
    tcase_add_test(predicate_case, wildcard_predicate);
    tcase_add_test(predicate_case, wildcard_predicate_on_mapping);
    tcase_add_test(predicate_case, wildcard_predicate_on_scalar);
    tcase_add_test(predicate_case, subscript_predicate);
    tcase_add_test(predicate_case, slice_predicate);
    tcase_add_test(predicate_case, subscript_predicate);
    tcase_add_test(predicate_case, slice_predicate_with_step);
    tcase_add_test(predicate_case, slice_predicate_negative_from);
    tcase_add_test(predicate_case, slice_predicate_copy);
    tcase_add_test(predicate_case, slice_predicate_reverse);

    TCase *recursive_case = tcase_create("recursive");
    tcase_add_unchecked_fixture(recursive_case, inventory_setup, evaluator_teardown);
    tcase_add_test(recursive_case, simple_recursive_step);
    tcase_add_test(recursive_case, compound_recursive_step);
    tcase_add_test(recursive_case, recursive_slice_predicate);
    tcase_add_test(recursive_case, recursive_subscript_predicate);
    tcase_add_test(recursive_case, recursive_slice_predicate_negative_from);
    tcase_add_test(recursive_case, recursive_wildcard);

    TCase *alias_case = tcase_create("alias");
    tcase_add_unchecked_fixture(alias_case, invoice_setup, evaluator_teardown);
    tcase_add_test(alias_case, name_alias);
    tcase_add_test(alias_case, type_alias);
    tcase_add_test(alias_case, greedy_wildcard_alias);
    tcase_add_test(alias_case, recursive_alias);
    tcase_add_test(alias_case, wildcard_predicate_alias);
    tcase_add_test(alias_case, recursive_wildcard_alias);
    
    Suite *evaluator = suite_create("Evaluator");
    suite_add_tcase(evaluator, bad_input_case);
    suite_add_tcase(evaluator, basic_case);
    suite_add_tcase(evaluator, predicate_case);
    suite_add_tcase(evaluator, recursive_case);
    suite_add_tcase(evaluator, alias_case);

    return evaluator;
}

