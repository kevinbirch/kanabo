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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


// String Entities

typedef struct string_s String;
typedef struct mutable_string_s MutableString;


// String Constructors

String *make_string(const char *value);
String *make_string_with_bytestring(const uint8_t *value, size_t length);

#define S(VALUE) make_string((VALUE))

String *string_clone(const String *self);


// String Destructor

void    string_free(String *self);


// String Data API

uint8_t string_get_char(const String *self, size_t index);


// String Iteration API

typedef bool (*string_iterator)(size_t index, uint8_t value, void *parameter);

bool string_iterate(const String *self, string_iterator iterator, void *parameter);

bool string_startswith(const String *self, const String *value);
bool string_startswith_c_string(const String *self, const char *value);

bool string_endswith(const String *self, const String *value);
bool string_endswith_c_string(const String *self, const char *value);

bool string_contains(const String *self, uint8_t value);


// String Attribute API

size_t string_get_length(const String *self);


// String Coercion API

const char *string_as_c_str(const String *self);


// Mutable String Constructors

MutableString *make_mstring(size_t capacity);
MutableString *make_mstring_with_char(const uint8_t value);
MutableString *make_mstring_with_c_str(const char *value);
MutableString *make_mstring_with_string(const String *value);

MutableString *mstring_clone(const MutableString *self);


// Mutable String Destructor

void           mstring_free(MutableString *self);


// Mutable String Attribute API

size_t   mstring_get_length(const MutableString *self);
size_t   mstring_get_capacity(const MutableString *self);
bool     mstring_has_capacity(const MutableString *self, size_t length);


// String Data API

uint8_t  mstring_get_char(const MutableString *self, size_t index);


// Mutable String Iteration API

bool mstring_iterate(const MutableString *self, string_iterator iterator, void *parameter);

bool mstring_startswith(const MutableString *self, const String *value);
bool mstring_startswith_c_string(const MutableString *self, const char *value);

bool mstring_endswith(const MutableString *self, const String *value);
bool mstring_endswith_c_string(const MutableString *self, const char *value);

bool mstring_contains(const MutableString *self, uint8_t value);


// Mutable String Coercion API

String        *mstring_as_string(const MutableString *self);
String        *mstring_as_string_no_copy(MutableString *self);
const char    *mstring_as_c_str(const MutableString *self);


// Mutable String Mutation API

bool mstring_append_char(MutableString **self, const uint8_t value);
bool mstring_append_c_str(MutableString **self, const char *value);
bool mstring_append_string(MutableString **self, const String *value);
bool mstring_append_mstring(MutableString **self, const MutableString *value);
bool mstring_append_stream(MutableString **self, const uint8_t *value, size_t length);

#define mstring_append(SELF, VALUE) _Generic((VALUE),                   \
                                             char *: mstring_append_c_str, \
                                             const char *: mstring_append_c_str, \
                                             char [sizeof(VALUE)]: mstring_append_c_str, \
                                             const char [sizeof(VALUE)]: mstring_append_c_str, \
                                             uint8_t: mstring_append_char, \
                                             const uint8_t: mstring_append_char, \
                                             String *: mstring_append_string, \
                                             const String *: mstring_append_string, \
                                             MutableString *: mstring_append_mstring, \
                                             const MutableString *: mstring_append_mstring \
                                             )(SELF, VALUE)

void mstring_set(MutableString *self, size_t position, uint8_t value);
void mstring_set_range(MutableString *self, size_t position, size_t length, const uint8_t *value);


// Generic API

#define string_length(SELF) _Generic((SELF),                            \
                                     String *: string_get_length,       \
                                     const String *: string_get_length, \
                                     MutableString *: mstring_get_length, \
                                     const MutableString *: mstring_get_length \
                                     )(SELF)
#define string_get(SELF, INDEX) _Generic((SELF),                        \
                                         String *: string_get_char,     \
                                         const String *: string_get_char, \
                                         MutableString *: mstring_get_char, \
                                         const MutableString *: mstring_get_char \
                                         )(SELF, INDEX)
#define string_as_c_string(SELF) _Generic((SELF),                       \
                                          String *: string_as_c_str,    \
                                          const String *: string_as_c_str, \
                                          MutableString *: mstring_as_c_str, \
                                          const MutableString *: mstring_as_c_str \
                                          )(SELF)
