#include <stdio.h>  // for fileno

#include <sys/stat.h>

#include "parser/input.h"


static off_t file_size(FILE *file)
{
    struct stat stats;
    if(fstat(fileno(file), &stats))
    {
        return -1;
    }

    return stats.st_size;
}

Input *make_input_from_file(const char *filename)
{
    Input *self = NULL;

    FILE *file = fopen(filename, "r");
    if(NULL == file)
    {
        goto exit;
    }

    off_t size = file_size(file);
    if(0 > size)
    {
        goto cleanup;
    }

    self = xcalloc(sizeof(Input) + (size_t)size);
    input_init(self, filename, (size_t)size);

    size_t count = fread(self->source.buffer, (size_t)size, 1, file);
    if(count != (size_t)size || ferror(file))
    {
        dispose_input(self);
        self = NULL;
    }

  cleanup:
    fclose(file);
  exit:
    return self;
}
