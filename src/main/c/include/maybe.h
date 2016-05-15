
#pragma once


#include <stdint.h>


enum maybe_tag_e
{
    NOTHING,
    JUST
};

typedef enum maybe_tag_e MaybeTag;
    
struct maybe_s
{
    MaybeTag tag;

    union
    {
        uint_fast16_t  code;
        void          *value;
    };
};

typedef struct maybe_s Maybe;


// Mabye Constructors

#define just(VALUE) (Maybe){.tag=JUST, .value=(VALUE)}
#define nothing() (Maybe){.tag=NOTHING, .code=0}


// Maybe functions

#define is_just(MAYBE) JUST == (MAYBE).tag
#define is_nothing(MAYBE) NOTHING == (MAYBE).tag

#define from_just(MAYBE) (MAYBE).value
#define from_nothing(MAYBE) (MAYBE).code
void *  from_maybe(void *def, Maybe a);

typedef void *(*maybe_fn)(void *a);
void *maybe(void *def, maybe_fn fn, Maybe a);


// Maybe Monand functions

// >>= function
typedef uint_fast16_t (*bind_fn)(void *a, void **result);
Maybe bind(Maybe a, bind_fn fn);

// >> function
typedef uint_fast16_t (*then_fn)(void **result);
Maybe then(Maybe a, then_fn fn);

#define inject(VALUE) just(VALUE)
#define fail(CODE) (Maybe){.tag=NOTHING, .code=(CODE)}


// Maybe MonadPlus functions

typedef uint_fast16_t (*mplus_fn)(void *a, void *b, void **result);
Maybe mplus(Maybe a, Maybe b, mplus_fn fn);

#define mzero() nothing()

#define define_maybe(NAME, TYPE) struct NAME##_maybe_s {                \
        MaybeTag tag;                                                   \
        union {uint_fast16_t code; TYPE value;};                        \
    };                                                                  \
    typedef struct NAME##_maybe_s (NAME);
