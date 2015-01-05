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
#include <string.h>

#include "evaluator/private.h"
#include "conditions.h"

#define nothing(CODE) (MaybeNodelist){.tag=NOTHING, .nothing={(CODE), evaluator_status_message((CODE))}}
#define just(NODELIST) (MaybeNodelist){.tag=JUST, .just=(NODELIST)}

#define PRECOND_ELSE_NOTHING(COND, CODE) ENSURE_THAT(nothing(CODE), EINVAL, (COND))
#define PRECOND_NONNULL_ELSE_NOTHING(VALUE, CODE) ENSURE_NONNULL(nothing(CODE), EINVAL, (VALUE))
#define PRECOND_NONZERO_ELSE_NOTHING(VALUE, CODE) ENSURE_THAT(nothing(CODE), EINVAL, 0 != (VALUE))


MaybeNodelist evaluate(const DocumentModel *model, const jsonpath *path)
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
