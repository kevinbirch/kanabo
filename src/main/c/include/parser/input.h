#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "str.h"
#include "parser/location.h"

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

struct source_location_s
{
    Input   *input;
    Location location;
};

typedef struct source_location_s SourceLocation;

// Input Constructors

Input *make_input_from_file(const char *filename);
Input *make_input_from_buffer(const char *data, size_t length);

void input_init(Input *self, const char *filename, size_t length);

// Input Destructor

void input_release(Input *self);
void dispose_input(Input *self);

// Input Property API

String *input_name(Input *self);
size_t  input_length(Input *self);
void    input_set_track_lines(Input *self, bool value);
bool    input_is_tracking_lines(Input *self);

// Input Postion API

Position input_position(const Input *self);
#define input_index(INPUT) input_position((INPUT)).index
#define input_line(INPUT) input_position((INPUT)).line
#define input_offset(INPUT) input_position((INPUT)).offset

void input_goto(Input *self, Position position);
void input_advance_to_end(Input *self);
void input_reset(Input *self);

// Input Query API

bool   input_has_more(Input *self);
size_t input_remaining(Input *self);

// Input Consumption API

char    input_peek(Input *self);
void    input_skip_whitespace(Input *self);
char    input_consume_one(Input *self);
size_t  input_consume_many(Input *self, size_t count, char *result);
bool    input_consume_if(Input *self, const char *value);
void    input_push_back(Input *self);
