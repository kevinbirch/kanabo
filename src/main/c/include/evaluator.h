#pragma once

#include "maybe.h"

#include "document.h"
#include "jsonpath.h"
#include "nodelist.h"

enum evaluator_error_e
{
    ERR_MODEL_IS_NULL,              // the model argument given was null
    ERR_PATH_IS_NULL,               // the path argument given was null
    ERR_NO_DOCUMENT_IN_MODEL,       // no document node was found in the model
    ERR_NO_ROOT_IN_DOCUMENT,        // no root node was found in the document
    ERR_UNEXPECTED_DOCUMENT_NODE,   // a document node was found embedded inside another document tree
    ERR_PATH_IS_EMPTY,              // the jsonpath given is empty
    ERR_SUBSCRIPT_PREDICATE,        // the subscript is too large for sequence
    ERR_SLICE_PREDICATE_DIRECTION,  // for [a:b:c] -> (c > 0 && a > b) || (c < 0 && a < b)
    ERR_SLICE_PREDICATE_ZERO_STEP,  // for [a:b:c] -> c == 0
    ERR_UNSUPPORTED_PATH,           // the jsonpath provided is not supported
};

typedef enum evaluator_error_e EvaluatorErrorCode;

defmaybep(Nodelist);

Maybe(Nodelist) evaluate(const DocumentSet *model, const JsonPath *path);

const char *evaluator_strerror(EvaluatorErrorCode code);
