#include <errno.h>
#include <string.h>

#include "evaluator/step.h"
#include "conditions.h"

Maybe(Nodelist) evaluate(const DocumentModel *model, const JsonPath *path)
{
    PRECOND_NONNULL_ELSE_FAIL(Nodelist, ERR_MODEL_IS_NULL, model);
    PRECOND_NONNULL_ELSE_FAIL(Nodelist, ERR_PATH_IS_NULL, path);
    PRECOND_NONNULL_ELSE_FAIL(Nodelist, ERR_NO_DOCUMENT_IN_MODEL, model_document(model, 0));
    PRECOND_NONNULL_ELSE_FAIL(Nodelist, ERR_NO_ROOT_IN_DOCUMENT, model_document_root(model, 0));
    if(0 == vector_length(path->steps))
    {
        return fail(Nodelist, ERR_PATH_IS_EMPTY);
    }

    Nodelist *results;
    EvaluatorErrorCode code = evaluate_steps(model, path, &results);
    if(NULL == results)
    {
        return fail(Nodelist, code);
    }

    return just(Nodelist, results);
}
