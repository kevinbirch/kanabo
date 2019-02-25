#include "loader.h"

static const char * const MESSAGES[] =
{
    [ERR_INTERNAL_CTX_NODE] = "loader: internal: unexpected kind for context node",
    [ERR_INTERNAL_LIBYAML] = "loader: internal: unknown error loading input file",
    [ERR_INPUT_IS_NULL] = "loader: internal: input is NULL",
    [ERR_INPUT_SIZE_IS_ZERO] = "loader: internal: input is zero length",
    [ERR_NO_DOCUMENTS_FOUND] = "loader: parsing input: no documents found",
    [ERR_READER_FAILED] = "loader: reading input",
    [ERR_SCANNER_FAILED] = "loader: scanning input",
    [ERR_PARSER_FAILED] = "loader: parsing input",
    [ERR_NON_SCALAR_KEY] = "loader: parsing input: non-scalar mapping key found",
    [ERR_NO_ANCHOR_FOR_ALIAS] = "loader: parsing input: no matching anchor found for alias",
    [ERR_ALIAS_LOOP] = "loader: parsing input: alias refers to an anchor that is an ancestor",
    [ERR_DUPLICATE_KEY] = "loader: parsing input: duplicate mapping key",    
};

const char *loader_strerror(LoaderErrorCode code)
{
    return MESSAGES[code];
}
