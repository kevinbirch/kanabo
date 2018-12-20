#include "input.h"

static const char * const ERRORS[] =
{
    [MISSING_FILENAME] = "filename argument is NULL",
    [OPEN_FAILED] = "open failed",
    [EMPTY_FILE] = "input file is empty",
    [READ_ERROR] = "read error",
};

const char *input_strerror(InputErrorCode code)
{
    return ERRORS[code];
}
