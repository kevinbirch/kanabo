#include "loader/error.h"
#include "xalloc.h"

void add_loader_error(Vector *errors, Position position, LoaderErrorCode code)
{
    LoaderError *err = xcalloc(sizeof(LoaderError));
    err->code = code;
    err->position = position;

    vector_append(errors, err);
}
