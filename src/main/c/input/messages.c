#include "input.h"

static const char * const ERRORS[] =
{
    [MISSING_FILENAME] = "input: internal error: filename argument is NULL",
    [OPEN_FAILED] = "open failed",
    [EMPTY_FILE] = "file is empty",
    [READ_ERROR] = "read error",
};

const char *input_strerror(InputErrorCode code)
{
    return ERRORS[code];
}
