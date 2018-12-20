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

Maybe(DocumentSet) load(Input *input, DuplicateKeyStrategy strategy)
{
    if(NULL == input)
    {
        Vector *errors = make_vector_with_capacity(1);
        add_error(errors, NO_POSITION, ERR_INPUT_IS_NULL);
        return fail(DocumentSet, errors);
    }

    return load_yaml(input, strategy);
}
