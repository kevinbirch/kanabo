#include "loader/error.h"
#include "xalloc.h"

void add_loader_error(Vector *errors, Position position, LoaderErrorCode code)
{
    LoaderError *err = xcalloc(sizeof(LoaderError));
    err->code = code;
    err->position = position;

    vector_append(errors, err);
}

void add_loader_error_with_extra(Vector *errors, Position position, LoaderErrorCode code, String *extra)
{
    LoaderError *err = xcalloc(sizeof(LoaderError));
    err->code = code;
    err->position = position;
    err->extra = extra;

    vector_append(errors, err);
}

static void freedom_iterator(void *each)
{
    LoaderError *err = each;

    if(NULL != err->extra)
    {
        dispose_string(err->extra);
    }
    free(err);
}

void loader_dispose_errors(Vector *errors)
{
    vector_destroy(errors, freedom_iterator);
}
