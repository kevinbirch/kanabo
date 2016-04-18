
#pragma once

struct source_s
{
    size_t   length;
    uint8_t  data[];
};

typedef struct source_s Source;

struct input_s
{
    Postion position;
    Postion mark;
};

typedef struct input_s Input;

#define super(SELF) &(SELF)->base
#define source(SELF) &(SELF)->source
#define position(SELF) (SELF)->position

void    reset(Input *self);
void    advance_to_end(Input *self, Source *source);

bool    has_more(Input *self, Source *source);
size_t  remaining(Input *self, Source *source);

void    set_mark(Input *self);
void    reset_to_mark(Input *self);

uint8_t peek(Input *self, Source *source);
void    push_back(Input *self, Source *source);
void    skip_whitespace(Input *self, Source *source);
uint8_t consume_one(Input *self, Source *source);
String *consume_many(Input *self, Source *source, size_t count);
bool    consume_if(Input *self, Source *source, const String *value);
