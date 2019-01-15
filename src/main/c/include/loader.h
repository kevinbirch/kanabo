#pragma once

#include <stdio.h>

#include "maybe.h"
#include "document.h"
#include "input.h"
#include "options.h"
#include "position.h"
#include "vector.h"

enum loader_error_code_e
{
    ERR_INTERNAL_CTX_NODE,   // internal error - non-complex context node
    ERR_INTERNAL_LIBYAML,    // internal error - unknown libyaml error
    ERR_INPUT_IS_NULL,       // the input argument given was NULL
    ERR_INPUT_SIZE_IS_ZERO,  // input length was 0
    ERR_NO_DOCUMENTS_FOUND,  // no documents in input stream
    ERR_READER_FAILED,       // unable to read from the input
    ERR_SCANNER_FAILED,      // unable to lexically analyze the input
    ERR_PARSER_FAILED,       // unable to parse the input
    ERR_NON_SCALAR_KEY,      // found a non-scalar mapping key
    ERR_NO_ANCHOR_FOR_ALIAS, // no anchor referenced by alias
    ERR_ALIAS_LOOP,          // the alias references an ancestor
    ERR_DUPLICATE_KEY,       // a duplicate mapping key was detected
};

typedef enum loader_error_code_e LoaderErrorCode;

struct loader_error_s
{
    LoaderErrorCode code;
    Position        position;
};

typedef struct loader_error_s LoaderError;

make_maybep_error(DocumentSet, Vector *);

Maybe(DocumentSet) load_yaml(Input *, DuplicateKeyStrategy);
Maybe(DocumentSet) load_yaml_from_stdin(DuplicateKeyStrategy);

const char *loader_strerror(LoaderErrorCode);
