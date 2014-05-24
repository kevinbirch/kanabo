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

#pragma once

#include "model.h"
#include "jsonpath.h"
#include "nodelist.h"

enum evaluator_status_code
{
    EVALUATOR_SUCCESS = 0,
    ERR_MODEL_IS_NULL,             // the model argument given was null
    ERR_PATH_IS_NULL,              // the path argument given was null
    ERR_NO_DOCUMENT_IN_MODEL,      // no document node was found in the model
    ERR_NO_ROOT_IN_DOCUMENT,       // no root node was found in the document
    ERR_PATH_IS_NOT_ABSOLUTE,      // the jsonpath given is not an absolute path
    ERR_PATH_IS_EMPTY,             // the jsonpath given is empty
    ERR_EVALUATOR_OUT_OF_MEMORY,   // unable to allocate memory
    ERR_UNEXPECTED_DOCUMENT_NODE,  // a document node was found embedded inside another document tree
    ERR_UNSUPPORTED_PATH,          // the jsonpath provided is not supported
};

typedef enum evaluator_status_code evaluator_status_code;

typedef struct evaluator_context evaluator_context;

evaluator_context *make_evaluator(const document_model *model, const jsonpath *path);
evaluator_status_code evaluator_status(const evaluator_context *context);

void evaluator_free(evaluator_context *context);

nodelist *evaluate(evaluator_context *context);

const char *evaluator_status_message(const evaluator_context *context);
