#pragma once

#include "maybe.h"
#include "vector.h"

#include "document.h"
#include "jsonpath.h"

/*
 * Nodelist API
 */

typedef Vector Nodelist;

#define make_nodelist make_vector
#define make_nodelist_of make_vector_of
#define nodelist_free vector_free

#define nodelist_length   vector_length
#define nodelist_is_empty vector_is_empty

#define nodelist_get    vector_get
void nodelist_add(Nodelist *list, Node *value);
void nodelist_set(Nodelist *list, Node *value, size_t index);

typedef bool (*nodelist_iterator)(Node *each, void *context);
bool nodelist_iterate(const Nodelist *list, nodelist_iterator iterator, void *context);

typedef bool (*nodelist_map_function)(Node *each, void *context, Nodelist *target);

Nodelist *nodelist_map(const Nodelist *list, nodelist_map_function function, void *context);
Nodelist *nodelist_map_into(const Nodelist *list, nodelist_map_function function, void *context, Nodelist *target);

enum evaluator_error_e
{
    ERR_MODEL_IS_NULL,             // the model argument given was null
    ERR_PATH_IS_NULL,              // the path argument given was null
    ERR_NO_DOCUMENT_IN_MODEL,      // no document node was found in the model
    ERR_NO_ROOT_IN_DOCUMENT,       // no root node was found in the document
    ERR_PATH_IS_NOT_ABSOLUTE,      // the jsonpath given is not an absolute path
    ERR_PATH_IS_EMPTY,             // the jsonpath given is empty
    ERR_UNEXPECTED_DOCUMENT_NODE,  // a document node was found embedded inside another document tree
    ERR_UNSUPPORTED_PATH,          // the jsonpath provided is not supported
};

typedef enum evaluator_error_e EvaluatorErrorCode;

make_maybep(Nodelist);

Maybe(Nodelist) evaluate(const DocumentModel *model, const JsonPath *path);

const char *evaluator_strerror(EvaluatorErrorCode code);
