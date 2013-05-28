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

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "loader.h"
#include "loader/private.h"
#include "conditions.h"

static const char * const MESSAGES[] =
{
    "Success.",
    "Input argument was NULL.",
    "Input argument was zero length.",
    "Unable to allocate memory.",
    "An error occured reading the input: %s at %zd.",
    "An error occured scanning the input: %s at line %ld, column %ld.",
    "An error occured parsing the input: %s at line %ld, column %ld.",
    "An unexpected error has occured."
};

loader_status_code interpret_yaml_error(yaml_parser_t *parser)
{
    switch (parser->error)
    {
        case YAML_NO_ERROR:
            return LOADER_SUCCESS;
        case YAML_MEMORY_ERROR:
            return ERR_LOADER_OUT_OF_MEMORY;
        case YAML_READER_ERROR:
            return ERR_READER_FAILED;
        case YAML_SCANNER_ERROR:
            return ERR_SCANNER_FAILED;
        case YAML_PARSER_ERROR:
            return ERR_PARSER_FAILED;
        default:
            return ERR_OTHER;
    }
}

char *loader_status_message(const loader_context * restrict context)
{
    PRECOND_NONNULL_ELSE_NULL(context);
    PRECOND_NONNULL_ELSE_NULL(context->parser);
    
    char *message = NULL;
    int result = 0;
    yaml_parser_t *parser = context->parser;
    switch (context->code)
    {
        case ERR_READER_FAILED:
            result = asprintf(&message, MESSAGES[context->code], parser->problem, parser->problem_offset);
            break;	
        case ERR_SCANNER_FAILED:
            result = asprintf(&message, MESSAGES[context->code], parser->problem, parser->problem_mark.line+1, parser->problem_mark.column+1);
            break;
        case ERR_PARSER_FAILED:
            result = asprintf(&message, MESSAGES[context->code], parser->problem, parser->problem_mark.line+1, parser->problem_mark.column+1);
            break;	
        default:
            message = strdup(MESSAGES[context->code]);
            break;
    }
    if(-1 == result)
    {
        message = NULL;
    }

    return message;
}
