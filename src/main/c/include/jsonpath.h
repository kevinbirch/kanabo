#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "maybe.h"

/* JSONPath Entity Types */

enum path_kind
{
    ABSOLUTE_PATH,
    RELATIVE_PATH
};

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

enum predicate_kind
{
    WILDCARD,
    SUBSCRIPT,
    SLICE,
    JOIN
};

 /* JSONPath Entities */

typedef struct step_s Step;
typedef struct predicate_s Predicate;

typedef struct jsonpath_s JsonPath;

enum jsonpath_parser_result_code_e
{
    ERR_EXPECTED_NAME_CHAR = ERR_PARSER_UNEXPECTED_VALUE + 1, // expected a name character
    ERR_CONTROL_CODE,                // forbidden control code in string/name
    ERR_ESCAPE_SEQUENCE,             // unuspported escape sequene
    ERR_EMPTY_PREDICATE,             // a predicate is empty
    ERR_UNBALANCED_PRED_DELIM,       // missing closing predicate delimiter `]'
    ERR_UNSUPPORTED_PRED_TYPE,       // unsupported predicate found
    ERR_EXTRA_JUNK_AFTER_PREDICATE,  // extra characters after valid predicate
    ERR_EXPECTED_NODE_TYPE_TEST,     // expected a node type test
    ERR_EXPECTED_INTEGER,            // expected an integer
    ERR_INVALID_NUMBER,              // invalid number
    ERR_STEP_CANNOT_BE_ZERO,         // slice step value must be non-zero
    ERR_CODE_MAX = ERR_STEP_CANNOT_BE_ZERO
};

typedef enum jsonpath_parser_result_code_e ResultCode;

struct maybe_jsonpath_s
{
    MaybeTag tag;

    union
    {
        struct
        {
            uint_fast16_t  code;
            size_t         position;
            char          *message;
        } error;
        JsonPath *value;
    };
};

typedef struct maybe_jsonpath_s MaybeJsonPath;

/* Parser API */

MaybeJsonPath read_path(const char *expression);

/* Destructor */

void path_free(MaybeJsonPath result);

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
