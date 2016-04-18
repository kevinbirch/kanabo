
#pragma once

#include <string.h>
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

uint8_t string_get(const String *self, size_t index);


// String Equality API

bool string_equals(const String *self, const String *other);
bool string_equals_c_string(const String *self, const char *other);
bool string_equals_bytestring(const String *self, const uint8_t *other, size_t length);


// String Iteration API

typedef bool (*string_iterator)(size_t index, uint8_t value, void *parameter);

bool string_iterate(const String *self, string_iterator iterator, void *parameter);

bool string_startswith(const String *self, const String *value);
bool string_startswith_c_string(const String *self, const char *value);

bool string_endswith(const String *self, const String *value);
bool string_endswith_c_string(const String *self, const char *value);

bool string_contains(const String *self, uint8_t value);


// String Attribute API

size_t string_length(const String *self);


// String Coercion API

const char *string_as_c_str(const String *self);


// Mutable String Constructors

MutableString *make_mstring(size_t capacity);
MutableString *make_mstring_with_char(const uint8_t value);
MutableString *make_mstring_with_c_str(const char *value);
MutableString *make_mstring_with_string(const String *value);

MutableString *mstring_clone(const MutableString *self);


// Mutable String Destructor

void mstring_free(MutableString *self);


// Mutable String Attribute API

size_t mstring_length(const MutableString *self);
size_t mstring_get_capacity(const MutableString *self);
bool   mstring_has_capacity(const MutableString *self, size_t length);


// Mutable String Data API

uint8_t  mstring_get(const MutableString *self, size_t index);


// Mutable String Equality API

bool mstring_equals(const MutableString *self, const MutableString *other);
bool mstring_equals_string(const MutableString *self, const String *other);
bool mstring_equals_c_string(const MutableString *self, const char *other);
bool mstring_equals_bytestring(const MutableString *self, const uint8_t *other, size_t length);


// Mutable String Iteration API

bool mstring_iterate(const MutableString *self, string_iterator iterator, void *parameter);

bool mstring_startswith(const MutableString *self, const String *value);
bool mstring_startswith_c_string(const MutableString *self, const char *value);

bool mstring_endswith(const MutableString *self, const String *value);
bool mstring_endswith_c_string(const MutableString *self, const char *value);

bool mstring_contains(const MutableString *self, uint8_t value);


// Mutable String Coercion API

String     *mstring_as_string(const MutableString *self);
String     *mstring_as_string_no_copy(MutableString *self);
const char *mstring_as_c_str(const MutableString *self);


// Mutable String Update API

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

#define strlen(SELF) _Generic((SELF),                                   \
                              String *: string_length,                  \
                              const String *: string_length,            \
                              MutableString *: mstring_length,          \
                              const MutableString *: mstring_length,    \
                              char *: strlen,                           \
                              const char *: strlen                      \
                              )(SELF)
#define stridx(SELF, INDEX) _Generic((SELF),                            \
                                     String *: string_get,              \
                                     const String *: string_get,        \
                                     MutableString *: mstring_get,      \
                                     const MutableString *: mstring_get \
                                     )(SELF, INDEX)
#define strequ(SELF, OTHER, ...) _Generic((SELF),                       \
                                          String *: _Generic((OTHER),   \
                                                             String *: string_equals, \
                                                             const String *: string_equals, \
                                                             char *: string_equals_c_string, \
                                                             const char *: string_equals_c_string, \
                                                             uint8_t *: string_equals_bytestring, \
                                                             const uint8_t *: string_equals_bytestring),         \
                                          const String *: _Generic((OTHER),   \
                                                                   String *: string_equals, \
                                                                   const String *: string_equals, \
                                                                   char *: string_equals_c_string, \
                                                                   const char *: string_equals_c_string, \
                                                                   uint8_t *: string_equals_bytestring, \
                                                                   const uint8_t *: string_equals_bytestring), \
                                          MutableString *: _Generic((OTHER),   \
                                                                    MutableString *: mstring_equals, \
                                                                    const MutableString *: mstring_equals, \
                                                                    String *: mstring_equals_string, \
                                                                    const String *: mstring_equals_string, \
                                                                    char *: mstring_equals_c_string, \
                                                                    const char *: mstring_equals_c_string, \
                                                                    uint8_t *: mstring_equals_bytestring, \
                                                                    const uint8_t *: mstring_equals_bytestring), \
                                          const MutableString *: _Generic((OTHER),   \
                                                                          MutableString *: mstring_equals, \
                                                                          const MutableString *: mstring_equals, \
                                                                          String *: mstring_equals_string, \
                                                                          const String *: mstring_equals_string, \
                                                                          char *: mstring_equals_c_string, \
                                                                          const char*: mstring_equals_c_string, \
                                                                          uint8_t *: mstring_equals_bytestring, \
                                                                          const uint8_t *: mstring_equals_bytestring) \
                                          )(SELF, OTHER, ##__VA_ARGS__)
#define striter(SELF, ITER, PARAM) _Generic((SELF),                     \
                                            String *: string_iterate,   \
                                            const String *: string_iterate, \
                                            MutableString *: mstring_iterate, \
                                            const MutableString *: mstring_iterate, \
                                            )(SELF, ITER, PARAM)
#define strbeg(SELF, TEST) _Generic((SELF),                             \
                                    String *: _Generic((OTHER),         \
                                                       String *: string_startswith, \
                                                       const String *: string_startswith, \
                                                       char *: string_startswith_c_string, \
                                                       const char *: string_startswith_c_string), \
\
                                    const String *: _Generic((OTHER),   \
                                                             String *: string_startswith, \
                                                             const String *: string_startswith, \
                                                             char *: string_startswith_c_string, \
                                                             const char *: string_startswith_c_string), \
                                    MutableString *: _Generic((OTHER),  \
                                                              MutableString *: mstring_startswith, \
                                                              const MutableString *: mstring_startswith, \
                                                              String *: mstring_startswith_string, \
                                                              const String *: mstring_startswith_string, \
                                                              char *: mstring_startswith_c_string, \
                                                              const char *: mstring_startswith_c_string), \
\
                                    const MutableString *: _Generic((OTHER), \
                                                                    MutableString *: mstring_startswith, \
                                                                    const MutableString *: mstring_startswith, \
                                                                    String *: mstring_startswith_string, \
                                                                    const String *: mstring_startswith_string, \
                                                                    char *: mstring_startswith_c_string, \
                                                                    const char*: mstring_startswith_c_string, \
ing) \
                                    )(SELF, TEST)
#define strend(SELF, TEST) _Generic((SELF),                             \
                                    String *: _Generic((OTHER),         \
                                                       String *: string_endswith, \
                                                       const String *: string_endswith, \
                                                       char *: string_endswith_c_string, \
                                                       const char *: string_endswith_c_string), \
\
                                    const String *: _Generic((OTHER),   \
                                                             String *: string_endswith, \
                                                             const String *: string_endswith, \
                                                             char *: string_endswith_c_string, \
                                                             const char *: string_endswith_c_string), \
                                    MutableString *: _Generic((OTHER),  \
                                                              MutableString *: mstring_endswith, \
                                                              const MutableString *: mstring_endswith, \
                                                              String *: mstring_endswith_string, \
                                                              const String *: mstring_endswith_string, \
                                                              char *: mstring_endswith_c_string, \
                                                              const char *: mstring_endswith_c_string), \
\
                                    const MutableString *: _Generic((OTHER), \
                                                                    MutableString *: mstring_endswith, \
                                                                    const MutableString *: mstring_endswith, \
                                                                    String *: mstring_endswith_string, \
                                                                    const String *: mstring_endswith_string, \
                                                                    char *: mstring_endswith_c_string, \
                                                                    const char*: mstring_endswith_c_string, \
ing) \
                                    )(SELF, TEST)

#define strcast(SELF) _Generic((SELF),                                  \
                               String *: string_as_c_str,               \
                               const String *: string_as_c_str,         \
                               MutableString *: mstring_as_c_str,       \
                               const MutableString *: mstring_as_c_str  \
                               )(SELF)
#define strclone(SELF) _Generic((SELF),                              \
                                String *: string_clone,              \
                                const String *: string_clone,        \
                                MutableString *: mstring_clone,      \
                                const MutableString *: mstring_clone \
                               )(SELF)
#define strdisp(SELF) _Generic((SELF),                                 \
                               String *: string_dispose,               \
                               const String *: string_dispose,         \
                               MutableString *: mstring_dispose,       \
                               const MutableString *: mstring_dispose  \
                               )(SELF)
