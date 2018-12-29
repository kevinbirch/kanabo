#include "loader.h"

static const char * const MESSAGES[] =
{
    [ERR_INPUT_IS_NULL] = "internal error: input is NULL",
    [ERR_INPUT_SIZE_IS_ZERO] = "internal error: input is zero length",
    [ERR_NO_DOCUMENTS_FOUND] = "no documents found",
    [ERR_READER_FAILED] = "error reading input",
    [ERR_SCANNER_FAILED] = "error scanning input",
    [ERR_PARSER_FAILED] = "error parsing input",
    [ERR_NON_SCALAR_KEY] = "non-scalar mapping key found",
    [ERR_NO_ANCHOR_FOR_ALIAS] = "no matching anchor found for alias",
    [ERR_ALIAS_LOOP] = "alias refers to an anchor that is an ancestor",
    [ERR_DUPLICATE_KEY] = "duplicate mapping key",    
};

const char *loader_strerror(LoaderErrorCode code)
{
    return MESSAGES[code];
}
