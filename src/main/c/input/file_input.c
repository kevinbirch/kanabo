#include <errno.h>
#include <stdio.h>  // for fileno
#include <sys/stat.h>

#include "conditions.h"
#include "input.h"

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
    ENSURE_NONNULL_ELSE_FAIL(Input, ((InputError){.code=OPEN_FAILED, .err=errno}), file);

    Maybe(Input) result;

    off_t size = file_size(file);
    if(0 > size)
    {
        result = fail(Input, ((InputError){.code=EMPTY_FILE, .err=errno}));
        goto cleanup;
    }

    Input *self = xcalloc(sizeof(Input) + (size_t)size);
    input_init(self, filename, (size_t)size);

    errno = 0;
    size_t count = fread(self->source.buffer, (size_t)size, 1, file);
    if(count != (size_t)size || ferror(file))
    {
        result = fail(Input, ((InputError){.code=READ_ERROR, .err=errno}));
        dispose_input(self);
        self = NULL;
        goto cleanup;
    }

    result = just(Input, self);

  cleanup:
    fclose(file);

    return result;
}
