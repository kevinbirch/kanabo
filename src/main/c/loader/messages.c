#include "loader.h"

static const char * const MESSAGES[] =
{
    [ERR_INTERNAL_CTX_NODE] = "loader: internal error: unexpected kind for context node",
    [ERR_INTERNAL_LIBYAML] = "loader: internal error: unknown error loading input file",
    [ERR_INPUT_IS_NULL] = "loader: internal error: input is NULL",
    [ERR_INPUT_SIZE_IS_ZERO] = "loader: internal error: input is zero length",
    [ERR_NO_DOCUMENTS_FOUND] = "no documents found",
    [ERR_READER_FAILED] = "reading input",
    [ERR_SCANNER_FAILED] = "scanning input",
    [ERR_PARSER_FAILED] = "parsing input",
    [ERR_NON_SCALAR_KEY] = "non-scalar mapping key found",
    [ERR_NO_ANCHOR_FOR_ALIAS] = "no matching anchor found for alias",
    [ERR_ALIAS_LOOP] = "alias refers to an anchor that is an ancestor",
    [ERR_DUPLICATE_KEY] = "duplicate mapping key",    
    [ERR_NUMBER_OUT_OF_RANGE] = "number literal cannot be represented",
    [ERR_BAD_TIMESTAMP] = "timestamp literal cannot be parsed",
};

const char *loader_strerror(LoaderErrorCode code)
{
    return MESSAGES[code];
}
