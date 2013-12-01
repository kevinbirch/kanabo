/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 *
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README file in the project root.
 *
 * Distributed under an [MIT-style][license] license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimers in the documentation and/or
 *   other materials provided with the distribution.
 * - Neither the names of the copyright holders, nor the names of the authors, nor
 *   the names of other contributors may be used to endorse or promote products
 *   derived from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/ncsa
 */

#include <errno.h>

#include "evaluator.h"
#include "evaluator/private.h"
#include "log.h"           

evaluator_context *make_evaluator(const document_model *model, const jsonpath *path)
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
    if(ABSOLUTE_PATH != path_kind(path))
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

