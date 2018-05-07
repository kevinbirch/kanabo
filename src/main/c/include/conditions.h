#pragma once

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

extern const void * SENTINEL;

#define VOID_RETURN 

#define ENSURE_NONNULL(ERR_RESULT, ERRNO, ...)                          \
    if(precond_is_null(__VA_ARGS__, SENTINEL))                          \
    {                                                                   \
        errno = 0 == errno ? (ERRNO) : errno;                           \
        return ERR_RESULT;                                              \
    }

#define ENSURE_THAT(ERR_RESULT, ERRNO, ...)                             \
    if(precond_is_false(__VA_ARGS__, -1))                               \
    {                                                                   \
        errno = 0 == errno ? (ERRNO) : errno;                           \
        return ERR_RESULT;                                              \
    }

// common preconditions
#define PRECOND_NONNULL_ELSE_NULL(...) ENSURE_NONNULL(NULL, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_FALSE(...) ENSURE_NONNULL(false, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_TRUE(...) ENSURE_NONNULL(true, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_VOID(...) ENSURE_NONNULL(VOID_RETURN, EINVAL, __VA_ARGS__)
#define PRECOND_NONNULL_ELSE_ZERO(...) ENSURE_NONNULL(0, EINVAL, __VA_ARGS__)

#define PRECOND_ELSE_NULL(...) ENSURE_THAT(NULL, EINVAL, __VA_ARGS__)
#define PRECOND_ELSE_FALSE(...) ENSURE_THAT(false, EINVAL, __VA_ARGS__)
#define PRECOND_ELSE_ZERO(...) ENSURE_THAT(0, EINVAL, __VA_ARGS__)
#define PRECOND_ELSE_VOID(...) ENSURE_THAT(VOID_RETURN, EINVAL, __VA_ARGS__)

// common invariants
#define ENSURE_NONNULL_ELSE_NULL(ERRNO, ...) ENSURE_NONNULL(NULL, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_FALSE(ERRNO, ...) ENSURE_NONNULL(false, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_TRUE(ERRNO, ...) ENSURE_NONNULL(true, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_VOID(ERRNO, ...) ENSURE_NONNULL(VOID_RETURN, ERRNO, __VA_ARGS__)
#define ENSURE_NONNULL_ELSE_ZERO(ERRNO, ...) ENSURE_NONNULL(0, ERRNO, __VA_ARGS__)

#define ENSURE_ELSE_NULL(ERRNO, ...) ENSURE_THAT(NULL, ERRNO, __VA_ARGS__)
#define ENSURE_ELSE_FALSE(ERRNO, ...) ENSURE_THAT(false, ERRNO, __VA_ARGS__)

bool precond_is_null(const void *first, ...);
bool precond_is_false(int first, ...);
