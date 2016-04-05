/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 *
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README file in the project root.
 *
 * Distributed under an [MIT-style][license] license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimers in the documentation and/or
 *   other materials provided with the distribution.
 * - Neither the names of the copyright holders, nor the names of the authors, nor
 *   the names of other contributors may be used to endorse or promote products
 *   derived from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/ncsa
 */

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

typedef struct input_s Input;
typedef struct file_input_s FileInput;
typedef struct buffer_input_s BufferInput;


// Input Constructors

FileInput   *make_file_input(const char *filename);
BufferInput *make_buffer_input(const uint8_t *data, size_t length);


// Input Destructor

void dispose_file_input(FileInput *self);
void dispose_buffer_input(BufferInput *self);

#define dispose_input(SELF) _Generic((SELF),                            \
                                     FileInput *: dispose_file_input,   \
                                     BufferInput *: dispose_buffer_input \
                                     )(SELF)


// Input Property API

String *file_input_name(FileInput *self);

// Input Coercion API

Input *file_input_upcast(FileInput *self);
Input *buffer_input_upcast(BufferInput *self);

#define input(SELF) _Generic((SELF),                            \
                             FileInput *: file_input_upcast,    \
                             BufferInput *: buffer_input_upcast \
                             )(SELF)
// Input Postion API

Postion input_position(const Input *self);

#define position(SELF) _Generic((SELF),                           \
                                FileInput *: input_position,      \
                                BufferInput *: input_position     \
                                )(input(SELF))

void    input_advance_to_end(Input *self);

#define advance_to_end(SELF) _Generic((SELF),                           \
                                      FileInput *: input_advance_to_end, \
                                      BufferInput *: input_advance_to_end \
                                      )(input(SELF))


// Input Mark API

void input_set_mark(Input *self);
#define set_mark(SELF) _Generic((SELF),                           \
                                FileInput *: input_set_mark,      \
                                BufferInput *: input_set_mark     \
                                )(input(SELF))

void input_reset_to_mark(Input *self);
#define reset_to_mark(SELF) _Generic((SELF),                            \
                                     FileInput *: input_reset_to_mark,  \
                                     BufferInput *: input_reset_to_mark \
                                     )(input(SELF))

void input_rewind(Input *self);
#define rewind(SELF) _Generic((SELF),                             \
                              FileInput *: input_rewind,          \
                              BufferInput *: input_rewind         \
                              )(input(SELF))


// Input Consumption API

uint8_t peek(Input *self);

void    file_input_skip_whitespace(FileInput *self);
void    buffer_input_skip_whitespace(BufferInput *self);

#define skip_whitespace(SELF) _Generic((SELF),                          \
                                       FileInput *: file_input_skip_whitespace, \
                                       BufferInput *: buffer_input_skip_whitespace \
                                       )(SELF)

uint8_t  consume_one(Input *self);
String  *consume_many(Input *self, size_t count);
bool     consume_if(Input *self, const uint8_t *value, size_t length);
void     push_back(Input *self);


// Input Query API

bool   has_more(Input *self);
size_t remaining(Input *self);

bool   looking_at(Input *self, const char *value);
size_t find(Input *self, const char *value);
