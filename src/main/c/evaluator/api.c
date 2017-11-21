#include <errno.h>

#include "evaluator.h"
#include "evaluator/private.h"
#include "log.h"

evaluator_context *make_evaluator(const document_model *model, const JsonPath *path)
{
    evaluator_debug("creating evaluator context");
    evaluator_context *context = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    if(NULL == context)
    {
        evaluator_debug("uh oh! out of memory, can't allocate the evaluator context");
        return NULL;
    }
    if(NULL == model)
    {
        evaluator_debug("model is null");
        errno = EINVAL;
        context->code = ERR_MODEL_IS_NULL;
        return context;
    }
    if(NULL == path)
    {
        evaluator_debug("path is null");
        errno = EINVAL;
        context->code = ERR_PATH_IS_NULL;
        return context;
    }
    if(NULL == model_document(model, 0))
    {
        evaluator_debug("document is null");
        errno = EINVAL;
        context->code = ERR_NO_DOCUMENT_IN_MODEL;
        return context;
    }
    if(NULL == model_document_root(model, 0))
    {
        evaluator_debug("document root is null");
        errno = EINVAL;
        context->code = ERR_NO_ROOT_IN_DOCUMENT;
        return context;
    }
    if(ABSOLUTE_PATH != path->kind)
    {
        evaluator_debug("path is not absolute");
        errno = EINVAL;
        context->code = ERR_PATH_IS_NOT_ABSOLUTE;
        return context;
    }
    if(0 == path_length(path))
    {
        evaluator_debug("path is empty");
        errno = EINVAL;
        context->code = ERR_PATH_IS_EMPTY;
        return context;
    }

    nodelist *list = make_nodelist();
    if(NULL == list)
    {
        evaluator_debug("uh oh! out of memory, can't allocate the result nodelist");
        context->code = ERR_EVALUATOR_OUT_OF_MEMORY;
        return context;
    }
    context->list = list;
    context->model = model;
    context->path = path;

    return context;
}

enum evaluator_status_code evaluator_status(const evaluator_context *context)
{
    return context->code;
}

void evaluator_free(evaluator_context *context)
{
    evaluator_debug("destroying evaluator context");
    if(NULL == context)
    {
        return;
    }
    context->model = NULL;
    context->path = NULL;
    context->list = NULL;

    free(context);
}
