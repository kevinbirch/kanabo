
#pragma once


struct source_s
{
    enum
    {
        BUFFER_SOURCE,
    } tag;
    size_t  length;
    String *name;
};

typedef struct source_s Source;

struct buffer_source_s
{
    union
    {
        struct source_s;
        Source base;
    };
    uint8_t  data[];
};

typedef struct buffer_source_s BufferSource;


#define source_get_value(SELF, INDEX) _Generic((SELF), \
                                               BufferSource *: (SELF)->data[(INDEX)] \
                                               )

#define source_cursor(SELF, INDEX) _Generic((SELF), \
                                            BufferSource *: (SELF)->data + (INDEX) \
                                            )


void buffer_source_init(BufferSource *self, const char *name, size_t size);

#define source_init(SELF, NAME, SIZE) _Generic((SELF), \
                                               BufferSource *: buffer_source_init \
                                               )(SELF, NAME, SIZE)
