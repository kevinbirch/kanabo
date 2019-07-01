#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "xalloc.h"

struct string_s
{
    size_t  length;
    uint8_t value[];
};

struct mutable_string_s
{
    size_t  capacity;
    String  base;
};


static inline String *string_alloc(size_t length)
{
    return xcalloc(sizeof(String) + length + 1);
}

static inline String *string_init(String *self, const char *value, size_t length)
{
    self->length = length;
    memcpy(self->value, value, length);
    self->value[self->length] = '\0';

    return self;
}

static inline String *string_init_with_bytestring(String *self, const uint8_t *value, size_t length)
{
    self->length = length;
    memcpy(self->value, value, length);
    self->value[self->length] = '\0';

    return self;
}

String *make_string(const char *value)
{
    if(NULL == value)
    {
        return NULL;
    }

    size_t length = strlen(value);
    String *self = string_alloc(length);

    return string_init(self, value, length);
}

String *make_string_with_bytestring(const uint8_t *value, size_t length)
{
    if(NULL == value)
    {
        return NULL;
    }

    String *self = string_alloc(length);

    return string_init_with_bytestring(self, value, length);
}

String *format(const char *format, ...)
{
    if(NULL == format)
    {
        return NULL;
    }

    va_list args;
    va_start(args, format);

    String *result = vformat(format, args);

    va_end(args);

    return result;
}

String *vformat(const char *format, va_list format_args)
{
    if(NULL == format)
    {
        return NULL;
    }

    va_list count_args;
    va_copy(count_args, format_args);

    errno = 0;
    int count = vsnprintf(NULL, 0, format, count_args);
    va_end(count_args);

    if(0 > count)
    {
        return NULL;
    }

    size_t length = (size_t)count;
    String *self = string_alloc(length);
    self->length = length;

    errno = 0;
    count = vsnprintf((char *)self->value, length + 1, format, format_args);  // N.B. - sizeof(self->value) == length + 1
    if(0 > count)
    {
        free(self);
        return NULL;
    }

    return self;
}

void dispose_string(String *self)
{
    if(NULL == self)
    {
        return;
    }

    free(self);
}

String *string_clone(const String *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    String *that = string_alloc(self->length);
    return string_init(that, (const char *)self->value, self->length);
}

size_t string_length(const String *self)
{
    if(NULL == self)
    {
        return 0;
    }

    return self->length;
}

const uint8_t * string_data(const String *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    return self->value;
}

uint8_t string_get(const String *self, size_t index)
{
    if(NULL == self)
    {
        return 0;
    }

    return self->value[index];
}

bool string_equals(const String *self, const String *other)
{
    if(NULL == self || NULL == other)
    {
        return false;
    }

    if(self->length != other->length)
    {
        return false;
    }

    return 0 == memcmp(self->value, other->value, self->length);
}

bool string_equals_c_string(const String *self, const char *other)
{
    if(NULL == self)
    {
        return false;
    }

    if(self->length != strlen(other))
    {
        return false;
    }

    return 0 == memcmp(self->value, other, self->length);
}

bool string_equals_bytestring(const String *self, const uint8_t *other, size_t length)
{
    if(NULL == self || NULL == other)
    {
        return false;
    }

    if(self->length != length)
    {
        return false;
    }

    return 0 == memcmp(self->value, other, self->length);
}

bool string_iterate(const String *self, string_iterator iterator, void *parameter)
{
    if(NULL == self)
    {
        return false;
    }

    for(size_t i = 0; i < string_length(self); i++)
    {
        if(!iterator(i, self->value[i], parameter))
        {
            return false;
        }
    }

    return true;
}

bool string_startswith(const String *self, const String *value)
{
    if(NULL == self || NULL == value)
    {
        return false;
    }

    if(value->length > self->length)
    {
        return false;
    }

    return 0 == memcmp(self->value, value->value, value->length);
}

bool string_startswith_c_string(const String *self, const char *value)
{
    if(NULL == self || NULL == value)
    {
        return false;
    }

    size_t length = strlen(value);
    if(length > self->length)
    {
        return false;
    }

    return 0 == memcmp(self->value, value, length);
}

bool string_endswith(const String *self, const String *value)
{
    if(NULL == self || NULL == value)
    {
        return false;
    }

    if(value->length > self->length)
    {
        return false;
    }

    size_t offset = self->length - value->length;

    return 0 == memcmp(self->value + offset, value->value, value->length);
}

bool string_endswith_c_string(const String *self, const char *value)
{
    if(NULL == self)
    {
        return false;
    }

    size_t length = strlen(value);
    if(length > self->length)
    {
        return false;
    }

    size_t offset = self->length - length;

    return 0 == memcmp(self->value + offset, value, length);
}

bool string_contains(const String *self, uint8_t value)
{
    if(NULL == self)
    {
        return false;
    }

    for(size_t i = 0; i < self->length; i++)
    {
        if(value == self->value[i])
        {
            return true;
        }
    }

    return false;
}

const char *string_as_c_str(const String *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    return (const char *)self->value;
}

char *string_copy(const String *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    char *result = xcalloc(self->length + 1);
    memcpy(result, self->value, self->length);
    result[self->length] = '\0';

    return result;
}

static inline size_t calculate_allocation_size(size_t capacity)
{
    return sizeof(MutableString) + capacity + 1;
}

static inline MutableString *mstring_alloc(const size_t capacity)
{
    return xcalloc(calculate_allocation_size(capacity));
}

static inline void mstring_realloc(MutableString **self, size_t capacity)
{
    *self = xrealloc(*self, calculate_allocation_size(capacity));
    (*self)->capacity = capacity;
}

static inline MutableString *mstring_init(MutableString *self, size_t capacity)
{
    self->base.length = 0;
    self->capacity = capacity;

    return self;
}

MutableString *make_mstring(size_t capacity)
{
    MutableString *self = mstring_alloc(capacity);
    return mstring_init(self, capacity);
}

MutableString *make_mstring_with_char(const uint8_t value)
{
    MutableString *self = make_mstring(1);
    mstring_append(&self, value);

    return self;
}

MutableString *make_mstring_with_c_str(const char *value)
{
    size_t length = strlen(value);
    MutableString *self = make_mstring(length);
    mstring_append(&self, value);

    return self;
}

MutableString *make_mstring_with_string(const String *value)
{
    MutableString *self = make_mstring(value->length);
    mstring_append(&self, value);

    return self;
}

void dispose_mstring(MutableString *self)
{
    if(NULL == self)
    {
        return;
    }

    free(self);
}

size_t mstring_length(const MutableString *self)
{
    if(NULL == self)
    {
        return 0;
    }

    return self->base.length;
}

const uint8_t * mstring_data(const MutableString *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    return self->base.value;
}

uint8_t mstring_get(const MutableString *self, size_t index)
{
    if(NULL == self)
    {
        return 0;
    }

    return self->base.value[index];
}

bool mstring_equals(const MutableString *self, const MutableString *other)
{
    if(NULL == self || NULL == other)
    {
        return false;
    }

    if(self->base.length != other->base.length)
    {
        return false;
    }

    return 0 == memcmp(self->base.value, other->base.value, self->base.length);
}

bool mstring_equals_string(const MutableString *self, const String *other)
{
    if(NULL == self || NULL == other)
    {
        return false;
    }

    if(self->base.length != other->length)
    {
        return false;
    }

    return 0 == memcmp(self->base.value, other->value, self->base.length);
}

bool mstring_equals_c_string(const MutableString *self, const char *other)
{
    if(NULL == self || NULL == other)
    {
        return false;
    }

    if(self->base.length != strlen(other))
    {
        return false;
    }

    return 0 == memcmp(self->base.value, other, self->base.length);
}

bool mstring_equals_bytestring(const MutableString *self, const uint8_t *other, size_t length)
{
    if(NULL == self || NULL == other)
    {
        return false;
    }

    if(self->base.length != length)
    {
        return false;
    }

    return 0 == memcmp(self->base.value, other, self->base.length);
}

bool mstring_iterate(const MutableString *self, string_iterator iterator, void *parameter)
{
    if(NULL == self)
    {
        return false;
    }

    return string_iterate(&self->base, iterator, parameter);
}

bool mstring_startswith(const MutableString *self, const String *value)
{
    if(NULL == self)
    {
        return false;
    }

    return string_startswith(&self->base, value);
}

bool mstring_startswith_c_string(const MutableString *self, const char *value)
{
    if(NULL == self)
    {
        return false;
    }

    return string_startswith_c_string(&self->base, value);
}

bool mstring_endswith(const MutableString *self, const String *value)
{
    if(NULL == self)
    {
        return false;
    }

    return string_endswith(&self->base, value);
}

bool mstring_endswith_c_string(const MutableString *self, const char *value)
{
    if(NULL == self)
    {
        return false;
    }

    return string_endswith_c_string(&self->base, value);
}

bool mstring_contains(const MutableString *self, uint8_t value)
{
    if(NULL == self)
    {
        return false;
    }

    return string_contains(&self->base, value);
}

size_t mstring_get_capacity(const MutableString *self)
{
    if(NULL == self)
    {
        return 0;
    }

    return self->capacity;
}

bool mstring_has_capacity(const MutableString *self, size_t length)
{
    if(NULL == self)
    {
        return false;
    }

    return (self->capacity - self->base.length) >= length;
}

MutableString *mstring_clone(const MutableString *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    MutableString *that = mstring_alloc(self->capacity);
    mstring_init(that, self->capacity);
    if(self->base.length)
    {
        memcpy(that->base.value, self->base.value, self->capacity + 1);
    }

    return that;
}

String *mstring_as_string(const MutableString *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    String *that = string_alloc(self->base.length);
    return string_init(that, (const char *)self->base.value, self->base.length);
}

String *mstring_as_string_no_copy(MutableString *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    return &self->base;
}

const char *mstring_as_c_str(const MutableString *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    return (const char *)self->base.value;
}

char *mstring_copy(const MutableString *self)
{
    if(NULL == self)
    {
        return NULL;
    }

    char *result = xcalloc(self->base.length + 1);
    memcpy(result, self->base.value, self->base.length);
    result[self->base.length] = '\0';

    return result;
}

static inline size_t calculate_new_capacity(size_t capacity)
{
    return (capacity * 3) / 2 + 1;
}

static inline void ensure_capacity(MutableString **self, size_t length)
{
    if(!mstring_has_capacity(*self, length))
    {
        size_t min_capacity = (*self)->base.length + length;
        size_t new_capacity = calculate_new_capacity(min_capacity);
        mstring_realloc(self, new_capacity);
    }
}

static inline void append(MutableString *self, const void *data, size_t length)
{
    memcpy(self->base.value + self->base.length, data, length);
    self->base.length += length;
    self->base.value[self->base.length] = '\0';
}

void mstring_append_byte(MutableString **self, const uint8_t value)
{
    if(NULL == self)
    {
        return;
    }

    ensure_capacity(self, 1);
    append(*self, &value, 1);
}

void mstring_append_char(MutableString **self, const char value)
{
    if(NULL == self)
    {
        return;
    }

    ensure_capacity(self, 1);
    append(*self, &value, 1);
}

void mstring_append_c_str(MutableString **self, const char *value)
{
    if(NULL == self)
    {
        return;
    }

    size_t length = strlen(value);
    ensure_capacity(self, length);
    append(*self, value, length);
}

void mstring_append_string(MutableString **self, const String *string)
{
    if(NULL == self)
    {
        return;
    }

    ensure_capacity(self, string->length);
    append(*self, string->value, string->length);
}

void mstring_append_mstring(MutableString **self, const MutableString *string)
{
    if(NULL == self)
    {
        return;
    }

    ensure_capacity(self, string->base.length);
    append(*self, string->base.value, string->base.length);
}

void mstring_append_stream(MutableString **self, const uint8_t *value, size_t length)
{
    if(NULL == self)
    {
        return;
    }

    ensure_capacity(self, length);
    append(*self, value, length);
}

bool mformat(MutableString **self, const char *format, ...)
{
    if(NULL == self)
    {
        return false;
    }

    va_list args;
    va_start(args, format);

    bool result = mvformat(self, format, args);

    va_end(args);

    return result;
}

bool mvformat(MutableString **self, const char *format, va_list format_args)
{
    if(NULL == self)
    {
        return false;
    }

    va_list count_args;
    va_copy(count_args, format_args);

    errno = 0;
    int count = vsnprintf(NULL, 0, format, count_args);
    va_end(count_args);

    if(0 > count)
    {
        return false;
    }

    size_t length = (size_t)count;
    ensure_capacity(self, length);
    
    char *end = ((char *)(*self)->base.value) + (*self)->base.length;
    errno = 0;
    count = vsnprintf(end, length + 1, format, format_args);  // N.B. - sizeof(self->value) == length + 1
    if(0 > count)
    {
        return false;
    }

    (*self)->base.length += length;

    return true;
}

void mstring_set(MutableString *self, size_t position, uint8_t value)
{
    if(NULL == self)
    {
        return;
    }

    if(position > self->base.length)
    {
        return;
    }
    self->base.value[position] = value;
}

void mstring_set_range(MutableString *self, size_t position, size_t length, const uint8_t *value)
{
    if(NULL == self)
    {
        return;
    }

    if(position > self->base.length || position + length > self->base.length)
    {
        return;
    }
    memcpy(self->base.value, value, length);
}
