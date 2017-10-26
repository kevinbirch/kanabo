#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "str.h"


// Input Entities

struct postion_s
{
    size_t index;
    size_t line;
    size_t offset;
};

typedef struct postion_s Position;

typedef struct input_s Input;


// Input Constructors

Input *make_input_from_file(const char *filename);
Input *make_input_from_buffer(const char *data, size_t length);

// Input Destructor

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

void input_advance_to_end(Input *self);
void input_reset(Input *self);


// Input Mark API

void input_push_mark(Input *self);
void input_reset_to_mark(Input *self);
void input_pop_mark(Input *self);
void input_drop_mark(Input *self);


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
