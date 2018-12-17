#ifdef __linux__
#define _POSIX_C_SOURCE 200809L /* for USE_POSIX feature macros */
#define _BSD_SOURCE             /* for S_IFREG flag on Linux */
#endif

#include <stdio.h>     // for fileno()
#include <sys/stat.h>  // for fstat()

#include "loader/debug.h"
#include "loader/error.h"
#include "loader/yaml.h"
#include "vector.h"

static const Position NO_POSITION = (Position){};

#define errcode(CODE) (LoaderError){code: (CODE)}

Maybe(DocumentSet) load(FILE *input, DuplicateKeyStrategy strategy)
{
    if(NULL == input)
    {
        Vector *errors = make_vector_with_capacity(1);
        add_error(errors, NO_POSITION, ERR_INPUT_IS_NULL);
        return fail(DocumentSet, errors);
    }

    struct stat file_info;
    if(-1 == fstat(fileno(input), &file_info))
    {
        Vector *errors = make_vector_with_capacity(1);
        add_error(errors, NO_POSITION, ERR_READER_FAILED);
        return fail(DocumentSet, errors);
    }

    if(!(file_info.st_mode & S_IFREG && (feof(input) || 0 == file_info.st_size)))
    {
        Vector *errors = make_vector_with_capacity(1);
        add_error(errors, NO_POSITION, ERR_INPUT_SIZE_IS_ZERO);
        return fail(DocumentSet, errors);
    }

    return load_yaml(input, strategy);
}
