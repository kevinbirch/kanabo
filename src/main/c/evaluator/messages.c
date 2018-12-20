#include "evaluator.h"

static const char * const ERRORS[] =
{
    [ERR_MODEL_IS_NULL] = "model argument is NULL",
    [ERR_PATH_IS_NULL] = "path argument is NULL",
    [ERR_NO_DOCUMENT_IN_MODEL] = "document node in model argument is NULL",
    [ERR_NO_ROOT_IN_DOCUMENT] = "root node of document in model argument is NULL",
    [ERR_PATH_IS_NOT_ABSOLUTE] = "not an absolute path",
    [ERR_PATH_IS_EMPTY] = "path has no steps",
    [ERR_UNEXPECTED_DOCUMENT_NODE] = "found a document node embedded in the tree",
    [ERR_UNSUPPORTED_PATH] = "the path is not supported",
};

const char *evaluator_strerror(EvaluatorErrorCode code)
{
    return ERRORS[code];
}
