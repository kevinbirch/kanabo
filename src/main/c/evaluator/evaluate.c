#include "conditions.h"
#include "evaluator/debug.h"
#include "evaluator/step.h"

Maybe(Nodelist) evaluate(const DocumentSet *documents, const JsonPath *path)
{
    PRECOND_NONNULL_ELSE_FAIL(Nodelist, ERR_MODEL_IS_NULL, documents);
    PRECOND_NONNULL_ELSE_FAIL(Nodelist, ERR_PATH_IS_NULL, path);
    PRECOND_ELSE_FAIL(Nodelist, ERR_NO_DOCUMENT_IN_MODEL, 0 != document_set_size(documents));
    PRECOND_NONNULL_ELSE_FAIL(Nodelist, ERR_NO_ROOT_IN_DOCUMENT, document_set_get_root(documents, 0));
    PRECOND_ELSE_FAIL(Nodelist, ERR_PATH_IS_EMPTY, 0 != vector_length(path->steps));

    String *repr = path_repr(path);
    evaluator_debug("evaluating path \"%s\"", C(repr));
    string_free(repr);

    Maybe(Nodelist) results = evaluate_steps(documents, path);

    return results;
}
