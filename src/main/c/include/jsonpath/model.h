#pragma once

#include "jsonpath.h"
#include "vector.h"


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

struct jsonpath_s
{
    enum path_kind kind;
    size_t length;
    Vector *steps;
};
