#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

enum maybe_tag_e
{
    NOTHING,
    JUST
};

typedef enum maybe_tag_e MaybeTag;
    
#define Maybe(TYPE) Maybe_ ## TYPE ## _s

#define defmaybe(TYPE)                          \
    typedef struct                              \
    {                                           \
        MaybeTag tag;                           \
        union                                   \
        {                                       \
            uint32_t error;                     \
            TYPE value;                         \
        };                                      \
    } Maybe(TYPE)

#define defmaybep(TYPE)                          \
    typedef struct                               \
    {                                            \
        MaybeTag tag;                            \
        union                                    \
        {                                        \
            uint32_t  error;                     \
            TYPE     *value;                     \
        };                                       \
    } Maybe(TYPE)

#define defmaybe_error(TYPE, ETYPE)               \
    typedef struct                                \
    {                                             \
        MaybeTag tag;                             \
        union                                     \
        {                                         \
            ETYPE error;                          \
            TYPE  value;                          \
        };                                        \
    } Maybe(TYPE)

#define defmaybep_error(TYPE, ETYPE)               \
    typedef struct                                 \
    {                                              \
        MaybeTag tag;                              \
        union                                      \
        {                                          \
            ETYPE  error;                          \
            TYPE  *value;                          \
        };                                         \
    } Maybe(TYPE)

// Mabye Constructors

#define just(a, x) (Maybe(a)){.tag=JUST, .value=(x)}
#define nothing(a) (Maybe(a)){.tag=NOTHING}
#define fail(a, v) (Maybe(a)){.tag=NOTHING, .error=(v)}

// Maybe functions

#define is_just(x) JUST == (x).tag
#define is_nothing(x) NOTHING == (x).tag

#define from_just(x) (x).value
#define from_nothing(x) (x).error
#define maybe(x, fallback) is_just(x) ? from_just(x) : fallback

// Maybe Monand functions

// the >>= function
#define bind(x, f) is_nothing(x) ? x : f(from_just(x))
    
// the >> function
#define then(x, y) is_just(x) && is_just(y) ? y : is_just(y) ? x : y

// Maybe MonadPlus functions

#define mplus(a, x, y) is_just(x) ? x : is_just(y) ? y : nothing(a)
#define mzero(a) nothing(a)

#ifdef __cplusplus
}
#endif
