#include <errno.h>
#include <string.h>

#include "evaluator/private.h"
#include "conditions.h"

#define PRECOND_ELSE_NOTHING(COND, CODE) ENSURE_THAT(nothing(CODE), EINVAL, (COND))
#define PRECOND_NONNULL_ELSE_NOTHING(VALUE, CODE) ENSURE_NONNULL(nothing(CODE), EINVAL, (VALUE))
#define PRECOND_NONZERO_ELSE_NOTHING(VALUE, CODE) ENSURE_THAT(nothing(CODE), EINVAL, 0 != (VALUE))

MaybeNodelist evaluate(const DocumentModel *model, const JsonPath *path)
{
    PRECOND_NONNULL_ELSE_NOTHING(model, ERR_MODEL_IS_NULL);
    PRECOND_NONNULL_ELSE_NOTHING(path, ERR_PATH_IS_NULL);
    PRECOND_NONNULL_ELSE_NOTHING(model_document(model, 0), ERR_NO_DOCUMENT_IN_MODEL);
    PRECOND_NONNULL_ELSE_NOTHING(model_document_root(model, 0), ERR_NO_ROOT_IN_DOCUMENT);
    PRECOND_ELSE_NOTHING(ABSOLUTE_PATH == path_kind(path), ERR_PATH_IS_NOT_ABSOLUTE);
    PRECOND_NONZERO_ELSE_NOTHING(path_length(path), ERR_PATH_IS_EMPTY);

    nodelist *list = NULL;
    evaluator_status_code code = evaluate_steps(model, path, &list);
    if(EVALUATOR_SUCCESS != code)
    {
        nodelist_free(list);
        return nothing(code);
    }
    return just(list);
}
