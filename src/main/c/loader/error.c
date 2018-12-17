#include "loader/error.h"

void add_error(Vector *errors, Position position, LoaderErrorCode code)
{
    LoaderError *err = xcalloc(sizeof(LoaderError));
    err->code = code;
    err->position = position;

    vector_append(errors, err);
}
