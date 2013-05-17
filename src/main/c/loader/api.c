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

#include <stdio.h>            /* for fileno() */
#include <sys/stat.h>         /* for fstat() */

#include "conditions.h"
#include "loader.h"
#include "loader/private.h"

static loader_context *make_loader(void);

loader_context *make_string_loader(const unsigned char *input, size_t size)
{
    loader_debug("creating string loader context");
    loader_context *context = make_loader();
    if(NULL == context || LOADER_SUCCESS != context->code)
    {
        return context;
    }
    if(NULL == input)
    {
        loader_debug("input is null");
        context->code = ERR_INPUT_IS_NULL;
        errno = EINVAL;
        return context;
    }
    if(0 == size)
    {
        loader_debug("input is empty");
        context->code = ERR_INPUT_SIZE_IS_ZERO;
        errno = EINVAL;
        return context;
    }

    yaml_parser_set_input_string(context->parser, input, size);

    return context;
}

loader_context *make_file_loader(FILE * restrict input)
{
    loader_debug("creating file loader context");
    loader_context *context = make_loader();
    if(NULL == context || LOADER_SUCCESS != context->code)
    {
        return context;
    }
    if(NULL == input)
    {
        loader_debug("input is null");
        context->code = ERR_INPUT_IS_NULL;
        errno = EINVAL;
        return context;
    }
    struct stat file_info;
    if(-1 == fstat(fileno(input), &file_info))
    {
        loader_debug("fstat failed on input file");
        context->code = ERR_READER_FAILED;
        errno = EINVAL;
        return context;
    }
    if(feof(input) || 0 == file_info.st_size)
    {
        loader_debug("input is empty");
        context->code = ERR_INPUT_SIZE_IS_ZERO;
        errno = EINVAL;
        return context;
    }

    yaml_parser_set_input_file(context->parser, input);
    return context;
}

static loader_context *make_loader(void)
{
    loader_context *context = (loader_context *)calloc(1, sizeof(loader_context));
    if(NULL == context)
    {
        loader_debug("uh oh! out of memory, can't allocate the loader context");
        return NULL;
    }

    yaml_parser_t *parser = (yaml_parser_t *)calloc(1, sizeof(yaml_parser_t));
    if(NULL == parser)
    {
        loader_debug("uh oh! out of memory, can't allocate the yaml parser");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return context;
    }
    document_model *model = make_model(1);
    if(NULL == model)
    {
        loader_debug("uh oh! out of memory, can't allocate the document model");
        context->code = ERR_LOADER_OUT_OF_MEMORY;
        return context;
    }
    
    if(!yaml_parser_initialize(parser))
    {
        loader_debug("uh oh! can't initialize the yaml parser");
        context->code = interpret_yaml_error(parser);
        return context;
    }

    context->parser = parser;
    context->model = model;
    context->excursions = NULL;
    context->head = NULL;
    context->last = NULL;

    return context;
}

enum loader_status_code loader_status(const loader_context * restrict context)
{
    return context->code;
}

void loader_free(loader_context *context)
{
    loader_debug("destroying loader context");
    if(NULL == context)
    {
        return;
    }
    yaml_parser_delete(context->parser);
    context->parser = NULL;
    for(struct excursion *entry = context->excursions; NULL != entry; entry = context->excursions)
    {
        context->excursions = entry->next;
        free(entry);
    }
    context->excursions = NULL;
    for(struct cell *entry = context->head; NULL != entry; entry = context->head)
    {
        context->head = entry->next;
        free(entry);
    }
    context->head = NULL;
    context->last = NULL;
    context->model = NULL;

    free(context);
}

document_model *load(loader_context *context)
{
    PRECOND_NONNULL_ELSE_NULL(context);
    PRECOND_NONNULL_ELSE_NULL(context->parser);
    PRECOND_NONNULL_ELSE_NULL(context->model);

    loader_debug("starting load...");
    return build_model(context);
}

