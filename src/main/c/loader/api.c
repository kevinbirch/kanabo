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

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L /* for USE_POSIX feature macros */
#define _BSD_SOURCE             /* for S_IFREG flag on Linux */
#endif

#include <stdio.h>     // for fileno()
#include <sys/stat.h>  // for fstat()

#include "conditions.h"
#include "loader.h"
#include "loader/private.h"

#define make_regex(MEMBER, PATTERN) (0 == regcomp((MEMBER), (PATTERN), REG_EXTENDED | REG_NOSUB))

#define _nothing(CODE, MESSAGE) (MaybeDocument){.tag=NOTHING, .nothing={(CODE), (MESSAGE)}}

#define nothing(CONTEXT) _nothing((CONTEXT)->code, loader_status_message((CONTEXT)))
#define just(MODEL) (MaybeDocument){.tag=JUST, .just=(MODEL)}

#define PRECOND_ELSE_NOTHING(COND, CODE) ENSURE_THAT(_nothing(CODE, loader_simple_status_message(CODE)), EINVAL, (COND))
#define PRECOND_NONNULL_ELSE_NOTHING(VALUE, CODE) ENSURE_NONNULL(_nothing(CODE, loader_simple_status_message(CODE)), EINVAL, (VALUE))
#define PRECOND_NONZERO_ELSE_NOTHING(VALUE, CODE) ENSURE_THAT(_nothing(CODE, loader_simple_status_message(CODE)), EINVAL, 0 != (VALUE))

static const char * const DECIMAL_PATTERN = "^-?(0|([1-9][[:digit:]]*))([.][[:digit:]]+)?([eE][+-]?[[:digit:]]+)?$";
static const char * const INTEGER_PATTERN = "^-?(0|([1-9][[:digit:]]*))$";
static const char * const TIMESTAMP_PATTERN = "^[0-9][0-9][0-9][0-9]-[0-9][0-9]?-[0-9][0-9]?(([Tt]|[ \t]+)[0-9][0-9]?:[0-9][0-9](:[0-9][0-9])?([.][0-9]+)?([ \t]*(Z|([-+][0-9][0-9]?(:[0-9][0-9])?)))?)?$";

static loader_status_code make_loader(loader_context *context, enum loader_duplicate_key_strategy value)
{
    loader_debug("creating common loader context");
    context->strategy = value;

    if(!yaml_parser_initialize(&context->parser))
    {
        return interpret_yaml_error(&context->parser);
    }

    context->anchors = make_hashtable_with_function(string_comparitor, fnv1a_string_hash);
    if(NULL == context->anchors)
    {
        return ERR_LOADER_OUT_OF_MEMORY;
    }

    if(!make_regex(&context->decimal_regex, DECIMAL_PATTERN))
    {
        hashtable_free(context->anchors);
        return ERR_OTHER;
    }

    if(!make_regex(&context->integer_regex, INTEGER_PATTERN))
    {
        hashtable_free(context->anchors);
        regfree(&context->decimal_regex);
        return ERR_OTHER;
    }

    if(!make_regex(&context->timestamp_regex, TIMESTAMP_PATTERN))
    {
        hashtable_free(context->anchors);
        regfree(&context->decimal_regex);
        regfree(&context->integer_regex);
        return ERR_OTHER;
    }

    return LOADER_SUCCESS;
}

static void loader_free(loader_context *context)
{
    loader_debug("destroying loader context...");

    yaml_parser_delete(&context->parser);

    hashtable_free(context->anchors);
    context->anchors = NULL;

    regfree(&context->decimal_regex);
    regfree(&context->integer_regex);
    regfree(&context->timestamp_regex);
}

static inline MaybeDocument load(loader_context *context)
{
    loader_debug("starting load...");
    build_model(context);
    if(LOADER_SUCCESS != context->code)
    {
        return nothing(context);
    }
    else
    {
        return just(context->model);
    }
}

MaybeDocument load_string(const unsigned char *input, size_t size, enum loader_duplicate_key_strategy value)
{
    PRECOND_NONNULL_ELSE_NOTHING(input, ERR_INPUT_IS_NULL);
    PRECOND_NONZERO_ELSE_NOTHING(size, ERR_INPUT_SIZE_IS_ZERO);

    loader_debug("creating string loader context");
    loader_context context;
    memset(&context, 0, sizeof(loader_context));

    loader_status_code code = make_loader(&context, value);
    if(LOADER_SUCCESS != code)
    {
        return nothing(&context);
    }

    yaml_parser_set_input_string(&context.parser, input, size);
    MaybeDocument result = load(&context);
    loader_free(&context);
    return result;
}

MaybeDocument load_file(FILE *input, enum loader_duplicate_key_strategy value)
{
    PRECOND_NONNULL_ELSE_NOTHING(input, ERR_INPUT_IS_NULL);

    struct stat file_info;
    int syscall_result = fstat(fileno(input), &file_info);
    PRECOND_ELSE_NOTHING(-1 != syscall_result, ERR_READER_FAILED);

    bool is_readable = !(file_info.st_mode & S_IFREG && (feof(input) || 0 == file_info.st_size));
    PRECOND_ELSE_NOTHING(is_readable, ERR_INPUT_SIZE_IS_ZERO);

    loader_debug("creating file loader context");
    loader_context context;
    memset(&context, 0, sizeof(loader_context));

    loader_status_code code = make_loader(&context, value);
    if(LOADER_SUCCESS != code)
    {
        return nothing(&context);
    }

    yaml_parser_set_input_file(&context.parser, input);
    MaybeDocument result = load(&context);
    loader_free(&context);
    return result;
}
