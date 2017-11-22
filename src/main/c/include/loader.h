#pragma once

#include <yaml.h>

#include "document.h"
#include "maybe.h"

enum loader_status_code
{
    LOADER_SUCCESS = 0,
    ERR_INPUT_IS_NULL,         // the input argument given was NULL
    ERR_INPUT_SIZE_IS_ZERO,    // input length was 0
    ERR_NO_DOCUMENTS_FOUND,    // no documents in input stream
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

typedef enum loader_status_code loader_status_code;

enum loader_duplicate_key_strategy
{
    DUPE_CLOBBER,
    DUPE_WARN,
    DUPE_FAIL
};

const char *duplicate_strategy_name(enum loader_duplicate_key_strategy value);
int32_t     parse_duplicate_strategy(const char *value);

struct maybe_document_s
{
    enum maybe_tag tag;
    union
    {
        DocumentModel *just;
        struct
        {
            loader_status_code code;
            char *message;
        } nothing;
    };
};

typedef struct maybe_document_s MaybeDocument;

MaybeDocument load_string(const unsigned char *input, size_t size, enum loader_duplicate_key_strategy value);
MaybeDocument load_file(FILE *input, enum loader_duplicate_key_strategy value);
