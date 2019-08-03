#ifdef __linux__
#define _POSIX_C_SOURCE 200112L  // for fileno
#endif

#include <errno.h>
#include <stdio.h>  // for fileno
#ifdef __linux__
#include <sys/types.h>  // for off_t
#endif
#include <sys/stat.h>  // for fstat

#include "conditions.h"
#include "input.h"
#include "xalloc.h"

static inline off_t file_size(FILE *file)
{
    errno = 0;
    struct stat stats;
    if(fstat(fileno(file), &stats))
    {
        return -1;
    }

    return stats.st_size;
}

Maybe(Input) make_input_from_file(const char *filename)
{
    ENSURE_NONNULL_ELSE_FAIL(Input, ((InputError){MISSING_FILENAME, EINVAL}), filename);

    errno = 0;
    FILE *file = fopen(filename, "r");
    ENSURE_NONNULL_ELSE_FAIL(Input, ((InputError){.code=OPEN_FAILED, .errno_val=errno}), file);

    Maybe(Input) result;

    off_t size = file_size(file);
    if(0 > size)
    {
        result = fail(Input, ((InputError){.code=EMPTY_FILE, .errno_val=errno}));
        goto cleanup;
    }

    Input *self = xcalloc(sizeof(Input) + (size_t)size);
    input_init(self, filename, (size_t)size);
    self->source.cache = true;

    errno = 0;
    size_t count = fread(self->source.buffer, (size_t)size, 1, file);
    if(1 != count || ferror(file))
    {
        result = fail(Input, ((InputError){.code=READ_ERROR, .errno_val=errno}));
        dispose_input(self);
        self = NULL;
        goto cleanup;
    }

    result = just(Input, self);

  cleanup:
    fclose(file);

    return result;
}
