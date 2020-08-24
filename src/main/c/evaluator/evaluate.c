#include "conditions.h"
#include "evaluator/debug.h"
#include "evaluator/step.h"

Maybe(Nodelist) evaluate(const DocumentSet *documents, const JsonPath *path)
{
    ENSURE_NONNULL_ELSE_FAIL(Nodelist, ERR_MODEL_IS_NULL, documents);
    ENSURE_NONNULL_ELSE_FAIL(Nodelist, ERR_PATH_IS_NULL, path);
    ENSURE_ELSE_FAIL(Nodelist, ERR_NO_DOCUMENT_IN_MODEL, 0 != document_set_size(documents));
    ENSURE_NONNULL_ELSE_FAIL(Nodelist, ERR_NO_ROOT_IN_DOCUMENT, document_set_get_root(documents, 0));
    ENSURE_ELSE_FAIL(Nodelist, ERR_PATH_IS_EMPTY, 0 != vector_length(path->steps));

    return evaluate_steps(documents, path);
}
