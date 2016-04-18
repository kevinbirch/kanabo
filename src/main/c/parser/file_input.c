
#include <stdio.h>  // for fileno

#include <sys/stat.h>

#include "parser/input.h"
#include "parser/input_base.h"


struct file_input_s
{
    union
    {
        struct input_s;
        Input base;
    };
    String *filename;
    Source  source;
};


static off_t file_size(FILE *file)
{
    struct stat stats;
    if(fstat(fileno(file), &stats))
    {
        return -1;
    }

    return stats.st_size;
}

static inline FileInput *file_input_alloc(size_t bufsize)
{
    return calloc(1, sizeof(FileInput) + bufsize);
}

static inline void file_input_init(FileInput *self, const char *filename, FILE *file, size_t size)
{
    self->filename = make_string(filename);
    size_t count = fread(self->source.data, 1, size, file);
    self->source.length = count;
}

FileInput *make_file_input(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if(NULL == file)
    {
        return NULL;
    }
    off_t size = file_size(file);
    if(0 > size)
    {
        return NULL;
    }
    
    FileInput *self = file_input_alloc((size_t)size);
    if(NULL == self)
    {
        fclose(file);
        return NULL;
    }
    file_input_init(self, filename, file, (size_t)size);

    return self;
}

void dispose_file_input(FileInput *self)
{
    string_free(self->filename);
    free(self);
}

String *file_name(FileInput *self)
{
    return self->filename;
}

Postion file_position(const FileInput *self)
{
    return position(super(self));
}

void file_advance_to_end(FileInput *self)
{
    advance_to_end(super(self), source(self));
}

void file_rewind(FileInput *self)
{
    reset(super(self));
}

void file_set_mark(FileInput *self)
{
    set_mark(super(self));
}

void file_reset_to_mark(FileInput *self)
{
    reset_to_mark(super(self));
}

bool file_has_more(FileInput *self)
{
    return has_more(super(self), source(self));
}

size_t file_remaining(FileInput *self)
{
    return remaining(super(self), source(self));
}

uint8_t file_peek(FileInput *self)
{
    return peek(super(self), source(self));
}

void file_skip_whitespace(FileInput *self)
{
    skip_whitespace(super(self), source(self));
}

uint8_t file_consume_one(FileInput *self)
{
    return consume_one(super(self), source(self));
}

String *file_consume_many(FileInput *self, size_t count)
{
    return consume_many(super(self), source(self), count);
}

bool file_consume_if(FileInput *self, const String *value)
{
    return consume_if(super(self), source(self), value);
}

void file_push_back(FileInput *self)
{
    push_back(super(self), source(self));
}

bool file_looking_at(FileInput *self, const char *value)
{
    return looking_at(super(self), source(self), value);
}
