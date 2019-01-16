#include "evaluator.h"

static const char * const ERRORS[] =
{
    [ERR_MODEL_IS_NULL] = "internal error: model argument is NULL",
    [ERR_PATH_IS_NULL] = "internal error: path argument is NULL",
    [ERR_NO_DOCUMENT_IN_MODEL] = "internal error: document node in model argument is NULL",
    [ERR_NO_ROOT_IN_DOCUMENT] = "internal error: root node of document in model argument is NULL",
    [ERR_UNEXPECTED_DOCUMENT_NODE] = "internal error: found a document node embedded in the tree",
    [ERR_SLICE_PREDICATE_DIRECTION] = "slice interval extents don't match step direction",
    [ERR_SLICE_PREDICATE_ZERO_STEP] = "slice interval step is zero",
    [ERR_PATH_IS_EMPTY] = "path has no steps",
    [ERR_UNSUPPORTED_PATH] = "the path is not supported",
};

const char *evaluator_strerror(EvaluatorErrorCode code)
{
    return ERRORS[code];
}
