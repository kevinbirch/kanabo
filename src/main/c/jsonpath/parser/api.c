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

#include "jsonpath.h"
#include "jsonpath/private.h"
#include "log.h"
#include "conditions.h"

parser_context *make_parser(const uint8_t *expression, size_t length)
{
    parser_debug("creating parser context");
    parser_context *context = (parser_context *)calloc(1, sizeof(parser_context));
    if(NULL == context)
    {
        return NULL;
    }
    if(NULL == expression)
    {
        context->result.code = ERR_NULL_EXPRESSION;
        errno = EINVAL;
        return context;
    }
    if(0 == length)
    {
        context->result.code = ERR_ZERO_LENGTH;
        errno = EINVAL;
        return context;
    }
    jsonpath *path = (jsonpath *)calloc(1, sizeof(jsonpath));
    if(NULL == path)
    {
        context->result.code = ERR_PARSER_OUT_OF_MEMORY;
        return context;
    }
    path->expression = (uint8_t *)calloc(1, length);
    if(NULL == path->expression)
    {
        return NULL;
    }
    memcpy(path->expression, expression, length);
    path->expr_length = length;

    context->steps = NULL;
    context->path = path;
    context->input = expression;
    context->length = length;
    context->cursor = 0;
    context->state = ST_START;
    context->path->kind = RELATIVE_PATH;
    context->result.code = JSONPATH_SUCCESS;
    context->result.expected_char = 0;
    context->result.actual_char = 0;

    return context;
}

enum parser_status_code parser_status(const parser_context *context)
{
    return context->result.code;
}

void parser_free(parser_context *context)
{
    parser_debug("destroying parser context");
    if(NULL == context)
    {
        return;
    }
    for(cell *entry = context->steps; NULL != entry; entry = context->steps)
    {
        context->steps = entry->next;
        free(entry);
    }
    context->steps = NULL;
    // N.B. - the path member should not be freed! it is given back to the caller of parse()
    context->path = NULL;
    context->input = NULL;
    free(context);
}

jsonpath *parse(parser_context *context)
{
    PRECOND_NONNULL_ELSE_NULL(context);
    PRECOND_NONNULL_ELSE_NULL(context->path);
    PRECOND_NONNULL_ELSE_NULL(context->input);
    PRECOND_ELSE_NULL(0 != context->length);

    debug_string("parsing expression: '%s'", context->input, context->length);

    if(parse_expression(context))
    {
        parser_debug("done. found %zd steps.", context->path->length);
        return context->path;
    }
    else
    {
        context->result.actual_char = context->input[context->cursor];
        path_free(context->path);
        context->path = NULL;
        return NULL;
    }
}
