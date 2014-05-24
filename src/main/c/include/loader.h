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

#include <stdio.h>
#include <yaml.h>

#include "model.h"

enum loader_status_code
{
    LOADER_SUCCESS = 0,
    ERR_INPUT_IS_NULL,         // the input argument given was NULL
    ERR_INPUT_SIZE_IS_ZERO,    // input length was 0
    ERR_LOADER_OUT_OF_MEMORY,  // unable to allocate memory
    ERR_READER_FAILED,         // unable to read from the input
    ERR_SCANNER_FAILED,        // unable to lexically analyze the input
    ERR_PARSER_FAILED,         // unable to parse the input
    ERR_NON_SCALAR_KEY,        // found a non-scalar mapping key
    ERR_NO_ANCHOR_FOR_ALIAS,   // no anchor referenced by alias
    ERR_ALIAS_LOOP,            // the alias references an ancestor
    ERR_DUPLICATE_KEY,         // a duplicate mapping key was detected
    ERR_OTHER
};

enum loader_duplicate_key_strategy
{
    DUPE_CLOBBER,
    DUPE_WARN,
    DUPE_FAIL
};

typedef enum loader_status_code loader_status_code;

typedef struct loader_context loader_context;

loader_context *make_string_loader(const unsigned char *input, size_t size);
loader_context *make_file_loader(FILE *input);
loader_status_code loader_status(const loader_context *context);

void loader_set_dupe_strategy(loader_context *context, enum loader_duplicate_key_strategy value);

void loader_free(loader_context *context);

document_model *load(loader_context *context);

char *loader_status_message(const loader_context *context);
