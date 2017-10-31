#pragma once

#include "document.h"
#include "jsonpath.h"
#include "nodelist.h"

enum evaluator_status_code
{
    EVALUATOR_SUCCESS = 0,
    ERR_MODEL_IS_NULL,             // the model argument given was null
    ERR_PATH_IS_NULL,              // the path argument given was null
    ERR_NO_DOCUMENT_IN_MODEL,      // no document node was found in the model
    ERR_NO_ROOT_IN_DOCUMENT,       // no root node was found in the document
    ERR_PATH_IS_NOT_ABSOLUTE,      // the jsonpath given is not an absolute path
    ERR_PATH_IS_EMPTY,             // the jsonpath given is empty
    ERR_EVALUATOR_OUT_OF_MEMORY,   // unable to allocate memory
    ERR_UNEXPECTED_DOCUMENT_NODE,  // a document node was found embedded inside another document tree
    ERR_UNSUPPORTED_PATH,          // the jsonpath provided is not supported
};

typedef enum evaluator_status_code evaluator_status_code;

typedef struct evaluator_context evaluator_context;

evaluator_context *make_evaluator(const document_model *model, const JsonPath *path);
evaluator_status_code evaluator_status(const evaluator_context *context);

void evaluator_free(evaluator_context *context);

nodelist *evaluate(evaluator_context *context);

const char *evaluator_status_message(const evaluator_context *context);

