#include "input.h"

static const char * const ERRORS[] =
{
    [MISSING_FILENAME] = "input: internal error: filename argument is NULL",
    [OPEN_FAILED] = "input: open failed",
    [EMPTY_FILE] = "input: file is empty",
    [READ_ERROR] = "input: read error",
};

const char *input_strerror(InputErrorCode code)
{
    return ERRORS[code];
}
