#pragma once

#include <stdio.h>
#include <yaml.h>

#include "document.h"

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


