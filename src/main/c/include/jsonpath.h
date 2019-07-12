#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "maybe.h"
#include "position.h"
#include "str.h"
#include "vector.h"

enum path_kind
{
    ABSOLUTE_PATH,
    RELATIVE_PATH
};

typedef enum path_kind PathKind;

struct jsonpath_s
{
    PathKind  kind;
    Vector   *steps;
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
    Position position;

    union
    {
        struct
        {
            int64_t index;
        } subscript;

        struct
        {
            uint8_t specified;
            int64_t from;
            int64_t to;
            int64_t step;
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
    Position position;

    struct
    {
        enum test_kind kind;

        union
        {
            String *name;
            enum type_test_kind type;
        };
    } test;

    Predicate *predicate;
};

typedef struct step_s Step;

/* Constructor */

JsonPath *make_jsonpath(PathKind kind);

/* Destructor */

void dispose_path(JsonPath *path);

/* Path API */

const char *        path_kind_name(enum path_kind value);
String *            path_repr(const JsonPath *path);

typedef bool        (*path_iterator)(Step *each, void *parser);
bool                path_iterate(const JsonPath *path, path_iterator iterator, void *context);

const char *        step_kind_name(enum step_kind value);
#define             test_kind(SELF) (SELF)->test.kind
const char *        test_kind_name(enum test_kind value);

/* Type Test API */

#define             type_test_step_kind(SELF) (TYPE_TEST == (SELF)->test.kind ? (SELF)->test.type : 0)
const char *        type_test_kind_name(enum type_test_kind value);

/* Name Test API */

#define             name_test_step_name(SELF) (NAME_TEST == (SELF)->test.kind ? (SELF)->test.name : NULL)
#define             name_test_step_length(SELF) (NAME_TEST == (SELF)->test.kind ? strlen((SELF)->test.name) : 0)

/* Predicate API */

const char *        predicate_kind_name(enum predicate_kind value);

/* Subscript Predicate API */

#define             subscript_predicate_index(SELF) (SUBSCRIPT == (SELF)->kind ? (SELF)->subscript.index : 0)

/* Slice Predicate API */

#define             slice_predicate_from(SELF) (SLICE == (SELF)->kind ? (SELF)->slice.from : (int64_t)0)
#define             slice_predicate_to(SELF) (SLICE == (SELF)->kind ? (SELF)->slice.to : (int64_t)0)
#define             slice_predicate_step(SELF) (SLICE == (SELF)->kind ? (((SELF)->slice.specified & SLICE_STEP) ? (SELF)->slice.step : (int64_t)1) : (int64_t)1)
#define             slice_predicate_has_from(SELF) (SLICE == (SELF)->kind ? (SELF)->slice.specified & SLICE_FROM : false)
#define             slice_predicate_has_to(SELF) (SLICE == (SELF)->kind ? (SELF)->slice.specified & SLICE_TO : false)
#define             slice_predicate_has_step(SELF) (SLICE == (SELF)->kind ? (SELF)->slice.specified & SLICE_STEP : false)

/* Join (Union) Predicate API */

#define             join_predicate_left(SELF) (JOIN == (SELF)->kind ? (SELF)->join.left : NULL)
#define             join_predicate_right(SELF) (JOIN == (SELF)->kind ? (SELF)->join.right : NULL)
