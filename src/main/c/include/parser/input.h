
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "str.h"


// Input Entities

struct postion_s
{
    size_t line;
    size_t offset;
};

typedef struct postion_s Postion;

typedef struct file_input_s FileInput;
typedef struct buffer_input_s BufferInput;


// Input Constructors

FileInput   *make_file_input(const char *filename);
BufferInput *make_buffer_input(const uint8_t *data, size_t length);


// Input Destructors

void dispose_file_input(FileInput *self);
void dispose_buffer_input(BufferInput *self);

#define dispose_input(SELF) _Generic((SELF),                            \
                                     FileInput *: dispose_file_input,   \
                                     BufferInput *: dispose_buffer_input \
                                     )(SELF)


// Input Property API

String *file_name(FileInput *self);
String *buffer_name(BufferInput *self);

#define input_name(SELF) _Generic((SELF),                     \
                                  FileInput *: file_name,     \
                                  BufferInput *: buffer_name  \
                                  )(input(SELF))


// Input Postion API

Postion file_position(const FileInput *self);
Postion buffer_position(const BufferInput *self);

#define input_position(SELF) _Generic((SELF),                        \
                                      FileInput *: file_position,    \
                                      BufferInput *: buffer_position \
                                      )(input(SELF))

void file_advance_to_end(FileInput *self);
void buffer_advance_to_end(BufferInput *self);

#define input_advance_to_end(SELF) _Generic((SELF),                     \
                                            FileInput *: file_advance_to_end, \
                                            BufferInput *: buffer_advance_to_end \
                                            )(input(SELF))


// Input Mark API

void file_set_mark(FileInput *self);
void buffer_set_mark(BufferInput *self);

#define input_set_mark(SELF) _Generic((SELF),                           \
                                      FileInput *: file_set_mark,       \
                                      BufferInput *: buffer_set_mark    \
                                      )(input(SELF))

void file_reset_to_mark(FileInput *self);
void buffer_reset_to_mark(BufferInput *self);

#define input_reset_to_mark(SELF) _Generic((SELF),                      \
                                           FileInput *: file_reset_to_mark, \
                                           BufferInput *: buffer_reset_to_mark \
                                           )(input(SELF))

void file_rewind(FileInput *self);
void buffer_rewind(BufferInput *self);

#define input_rewind(SELF) _Generic((SELF),                       \
                                    FileInput *: file_rewind,     \
                                    BufferInput *: buffer_rewind  \
                                    )(input(SELF))


// Input Consumption API

uint8_t file_peek(FileInput *self);
uint8_t buffer_peek(BufferInput *self);

#define input_peek(SELF) _Generic((SELF),                    \
                                  FileInput *: file_peek,    \
                                  BufferInput *: buffer_peek \
                                  )(SELF)

void file_skip_whitespace(FileInput *self);
void buffer_skip_whitespace(BufferInput *self);

#define input_skip_whitespace(SELF) _Generic((SELF),                    \
                                             FileInput *: file_skip_whitespace, \
                                             BufferInput *: buffer_skip_whitespace \
                                             )(SELF)

uint8_t file_consume_one(FileInput *self);
uint8_t buffer_consume_one(BufferInput *self);

#define input_consume_one(SELF) _Generic((SELF),                     \
                                  FileInput *: file_consumme_one,    \
                                  BufferInput *: buffer_consumme_one \
                                  )(SELF)

String *file_consume_many(FileInput *self, size_t count);
String *buffer_consume_many(BufferInput *self, size_t count);

#define input_consume_many(SELF) _Generic((SELF),                    \
                                  FileInput *: file_consume_many,    \
                                  BufferInput *: buffer_consume_many \
                                  )(SELF)

bool file_consume_if(FileInput *self, const uint8_t *value, size_t length);
bool buffer_consume_if(BufferInput *self, const uint8_t *value, size_t length);

#define input_consume_if(SELF) _Generic((SELF),                    \
                                  FileInput *: file_consume_if,    \
                                  BufferInput *: buffer_consume_if \
                                  )(SELF)

void file_push_back(FileInput *self);
void buffer_push_back(BufferInput *self);

#define input_push_back(SELF) _Generic((SELF),                    \
                                  FileInput *: file_push_back,    \
                                  BufferInput *: buffer_push_back \
                                  )(SELF)


// Input Query API

bool file_has_more(FileInput *self);
bool buffer_has_more(BufferInput *self);

#define input_has_more(SELF) _Generic((SELF),                    \
                                  FileInput *: file_has_more,    \
                                  BufferInput *: buffer_has_more \
                                  )(SELF)

size_t file_remaining(FileInput *self);
size_t buffer_remaining(BufferInput *self);

#define input_remaining(SELF) _Generic((SELF),                    \
                                  FileInput *: file_remaining,    \
                                  BufferInput *: buffer_remaining \
                                  )(SELF)

bool file_looking_at(FileInput *self, const char *value);
bool buffer_looking_at(BufferInput *self, const char *value);

#define input_looking_at(SELF) _Generic((SELF),                    \
                                  FileInput *: file_looking_at,    \
                                  BufferInput *: buffer_looking_at \
                                  )(SELF)

size_t file_find(FileInput *self, const char *value);
size_t buffer_find(BufferInput *self, const char *value);

#define input_find(SELF) _Generic((SELF),                    \
                                  FileInput *: file_find,    \
                                  BufferInput *: buffer_find \
                                  )(SELF)
