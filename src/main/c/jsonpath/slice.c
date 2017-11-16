#include "jsonpath.h"
#include "conditions.h"

static bool slice_predicate_has(const Predicate *value, enum slice_specifiers specifier);


int_fast32_t slice_predicate_to(const Predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.to;
}

int_fast32_t slice_predicate_from(const Predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.from;
}

int_fast32_t slice_predicate_step(const Predicate *value)
{
    PRECOND_NONNULL_ELSE_ZERO(value);
    PRECOND_ELSE_ZERO(SLICE == value->kind);
    return value->slice.step;
}

bool slice_predicate_has_to(const Predicate *value)
{
    return slice_predicate_has(value, SLICE_TO);
}

bool slice_predicate_has_from(const Predicate *value)
{
    return slice_predicate_has(value, SLICE_FROM);
}

bool slice_predicate_has_step(const Predicate *value)
{
    return slice_predicate_has(value, SLICE_STEP);
}

static bool slice_predicate_has(const Predicate *value, enum slice_specifiers specifier)
{
    PRECOND_NONNULL_ELSE_FALSE(value);
    PRECOND_ELSE_FALSE(SLICE == value->kind);
    return value->slice.specified & specifier;
}
