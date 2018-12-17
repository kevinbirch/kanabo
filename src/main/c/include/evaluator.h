#pragma once

#include "maybe.h"

#include "document.h"
#include "nodelist.h"
#include "jsonpath.h"

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

Maybe(Nodelist) evaluate(const DocumentSet *model, const JsonPath *path);

const char *evaluator_strerror(EvaluatorErrorCode code);
