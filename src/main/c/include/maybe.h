
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

#define just(VALUE) (Maybe){.tag=JUST, .value=(VALUE)}
#define nothing(CODE) (Maybe){.tag=NOTHING, .code=(CODE)}

#define is_nothing(MAYBE) NOTHING == (MAYBE).tag
#define is_value(MAYBE) JUST == (MAYBE).tag

#define value(MAYBE) (MAYBE).value
#define code(MAYBE) (MAYBE).code

#define define_maybe(NAME, TYPE) struct NAME##_maybe_s {                \
        MaybeTag tag;                                                   \
        union {uint_fast16_t code; TYPE value;};                        \
    };                                                                  \
    typedef struct NAME##_maybe_s (NAME);
