#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "vector.h"
#include "maybe.h"

enum path_kind
{
    ABSOLUTE_PATH,
    RELATIVE_PATH
};

struct jsonpath_s
{
    enum path_kind kind;
    size_t length;
    Vector *steps;
};

typedef struct jsonpath_s JsonPath;

enum predicate_kind
{
    WILDCARD,
    SUBSCRIPT,
    SLICE,
    JOIN
};

enum slice_specifiers
{
    SLICE_FROM = 1,
    SLICE_TO   = 2,
    SLICE_STEP = 4
};

struct predicate_s
{
    enum predicate_kind kind;

    union
    {
        struct
        {
            size_t index;
        } subscript;

        struct
        {
            uint8_t      specified;
            int_fast32_t from;
            int_fast32_t to;
            int_fast32_t step;
        } slice;

        struct
        {
            JsonPath *left;
            JsonPath *right;
        } join;
    };
};

typedef struct predicate_s Predicate;

enum step_kind
{
    ROOT,
    SINGLE,
    RECURSIVE
};

enum test_kind
{
    WILDCARD_TEST,
    NAME_TEST,
    TYPE_TEST
};

enum type_test_kind
{
    OBJECT_TEST,
    ARRAY_TEST,
    STRING_TEST,
    NUMBER_TEST,
    BOOLEAN_TEST,
    NULL_TEST,
};

struct step_s
{
    enum step_kind kind;

    struct
    {
        enum test_kind kind;

        union
        {
            struct
            {
                uint8_t *value;
                size_t  length;
            } name;

            enum type_test_kind type;
        };
    } test;

    Predicate *predicate;
};

typedef struct step_s Step;

/* Destructor */

void dispose_path(JsonPath path);

/* Path API */

enum path_kind      path_kind(const JsonPath *path);
const char *        path_kind_name(enum path_kind value);
size_t              path_length(const JsonPath *path);
Step *              path_get(const JsonPath *path, size_t index);

typedef bool (*path_iterator)(Step *each, void *parser);
bool path_iterate(const JsonPath *path, path_iterator iterator, void *context);

/* Step API */

enum step_kind      step_kind(const Step *value);
const char *        step_kind_name(enum step_kind value);
enum test_kind      step_test_kind(const Step *value);
const char *        test_kind_name(enum test_kind value);

/* Type Test API */

enum type_test_kind type_test_step_kind(const Step *value);
const char *        type_test_kind_name(enum type_test_kind value);

/* Name Test API */

uint8_t *           name_test_step_name(const Step *value);
size_t              name_test_step_length(const Step *value);

/* Predicate API */

bool                step_has_predicate(const Step *value);
Predicate *         step_predicate(const Step *value);

enum predicate_kind predicate_kind(const Predicate *value);
const char *        predicate_kind_name(enum predicate_kind value);

/* Subscript Predicate API */

size_t              subscript_predicate_index(const Predicate *value);

/* Slice Predicate API */

int_fast32_t        slice_predicate_to(const Predicate *value);
int_fast32_t        slice_predicate_from(const Predicate *value);
int_fast32_t        slice_predicate_step(const Predicate *value);
bool                slice_predicate_has_to(const Predicate *value);
bool                slice_predicate_has_from(const Predicate *value);
bool                slice_predicate_has_step(const Predicate *value);

/* Join (Union) Predicate API */

JsonPath *          join_predicate_left(const Predicate *value);
JsonPath *          join_predicate_right(const Predicate *value);
