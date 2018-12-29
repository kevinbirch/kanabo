#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "location.h"
#include "maybe.h"
#include "str.h"

// Input Entities

struct source_s
{
    size_t length;
    char   buffer[];
};

typedef struct source_s Source;

struct input_s
{
    String   *name;
    Position  position;
    bool      track_lines;
    Source    source;
};

typedef struct input_s Input;

enum input_error_e
{
    MISSING_FILENAME,
    OPEN_FAILED,
    EMPTY_FILE,
    READ_ERROR,
};

typedef enum input_error_e InputErrorCode;

struct input_error_s
{
    InputErrorCode code;
    int            err;
};

typedef struct input_error_s InputError;

// Input Constructors

make_maybep_error(Input, InputError);

Maybe(Input) make_input_from_file(const char *filename);
Input *make_input_from_buffer(const char *data, size_t length);

void input_init(Input *self, const char *filename, size_t length);

const char *input_strerror(InputErrorCode error);

// Input Destructor

void input_release(Input *self);
void dispose_input(Input *self);

// Input Property API

#define input_name(SELF) (SELF)->name
#define input_length(SELF) (SELF)->source.length
#define input_set_track_lines(SELF, VALUE) (SELF)->track_lines = (VALUE)
#define input_is_tracking_lines(SELF) (SELF)->track_lines == true

// Input Postion API

#define input_position(SELF) (SELF)->position
#define input_source_position(SELF) ((SourcePosition){(SELF)->name, (SELF)->position})
#define input_index(INPUT) input_position((INPUT)).index
#define input_line(INPUT) input_position((INPUT)).line
#define input_offset(INPUT) input_position((INPUT)).offset

void input_goto(Input *self, Position position);
#define input_advance_to_end(SELF) advance_by((SELF), input_remaining(SELF))
void input_reset(Input *self);

// Input Query API

#define input_has_more(SELF) ((SELF)->position.index < (SELF)->source.length)
size_t input_remaining(Input *self);

// Input Consumption API

#define input_peek(SELF) (SELF)->source.buffer[(SELF)->position.index]
void    input_skip_whitespace(Input *self);
char    input_consume_one(Input *self);
size_t  input_consume_many(Input *self, size_t count, char *result);
bool    input_consume_if(Input *self, const char *value);
void    input_push_back(Input *self);

char   *input_extract(Input *self, Location location);
