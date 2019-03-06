#include "evaluator.h"

static const char * const ERRORS[] =
{
    [ERR_MODEL_IS_NULL] = "evaluator: internal error: model argument is NULL",
    [ERR_PATH_IS_NULL] = "evaluator: internal error: path argument is NULL",
    [ERR_NO_DOCUMENT_IN_MODEL] = "evaluator: internal error: document node in model argument is NULL",
    [ERR_NO_ROOT_IN_DOCUMENT] = "evaluator: internal error: root node of document in model argument is NULL",
    [ERR_UNEXPECTED_DOCUMENT_NODE] = "evaluator: internal error: found a document node embedded in the tree",
    [ERR_PATH_IS_EMPTY] = "evaluator: evaluating expression: path has no steps",
    [ERR_SUBSCRIPT_PREDICATE] = "evaluator: evaluating expression: subscript index is out of range",
    [ERR_SLICE_PREDICATE_DIRECTION] = "evaluator: evaluating expression: slice interval extents don't match step direction",
    [ERR_SLICE_PREDICATE_ZERO_STEP] = "evaluator: evaluating expression: slice interval step is zero",
    [ERR_UNSUPPORTED_PATH] = "evaluator: evaluating expression: path is not supported",
};

const char *evaluator_strerror(EvaluatorErrorCode code)
{
    return ERRORS[code];
}
