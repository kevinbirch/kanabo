#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

extern const void * SENTINEL;

#define VOID_RETURN 

#define ENSURE_NONNULL(ERR_RESULT, ...)                                 \
    if(cond_is_null(__VA_ARGS__, SENTINEL))                             \
    {                                                                   \
        return ERR_RESULT;                                              \
    }

#define ENSURE_THAT(ERR_RESULT, ...)                                    \
    if(cond_is_false(__VA_ARGS__, -1))                                  \
    {                                                                   \
        return ERR_RESULT;                                              \
    }

// common preconditions

#define PRECOND_NONNULL_ELSE_NULL(...) ENSURE_NONNULL(NULL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_FALSE(...) ENSURE_NONNULL(false, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_TRUE(...) ENSURE_NONNULL(true, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_VOID(...) ENSURE_NONNULL(VOID_RETURN, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_ZERO(...) ENSURE_NONNULL(0, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_FAIL(X, ERR, ...)                          \
    if(cond_is_null(__VA_ARGS__, SENTINEL))                             \
    {                                                                   \
        return fail(X, ERR);                                            \
    }

#define PRECOND_ELSE_NULL(...) ENSURE_THAT(NULL, __VA_ARGS__)
#define PRECOND_ELSE_FALSE(...) ENSURE_THAT(false, __VA_ARGS__)
#define PRECOND_ELSE_ZERO(...) ENSURE_THAT(0, __VA_ARGS__)
#define PRECOND_ELSE_VOID(...) ENSURE_THAT(VOID_RETURN, __VA_ARGS__)
#define PRECOND_ELSE_FAIL(X, ERR, ...)                                  \
    if(cond_is_false(__VA_ARGS__, -1))                                  \
    {                                                                   \
        return fail(X, (ERR));                                          \
    }

// common invariants

#define ENSURE_NONNULL_ELSE_NULL(...) ENSURE_NONNULL(NULL, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_FALSE(...) ENSURE_NONNULL(false, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_TRUE(...) ENSURE_NONNULL(true, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_VOID(...) ENSURE_NONNULL(VOID_RETURN, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_ZERO(...) ENSURE_NONNULL(0, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_FAIL(X, ERR, ...)                           \
    if(cond_is_null(__VA_ARGS__, SENTINEL))                             \
    {                                                                   \
        return fail(X, ERR);                                            \
    }

#define ENSURE_ELSE_NULL(...) ENSURE_THAT(NULL, __VA_ARGS__)
#define ENSURE_ELSE_FALSE(...) ENSURE_THAT(false, __VA_ARGS__)
#define ENSURE_ELSE_FAIL(X, ERR, ...)                                   \
    if(cond_is_false(__VA_ARGS__, -1))                                  \
    {                                                                   \
        return fail(X, (ERR));                                          \
    }

bool cond_is_null(const void *first, ...);
bool cond_is_false(int first, ...);
