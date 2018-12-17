#pragma once

#include <stdbool.h>

#include "document.h"

bool emit_node(Node *value, void *context);
bool emit_scalar(const Scalar *);
bool emit_quoted_scalar(const Scalar *);
bool emit_raw_scalar(const Scalar *);
bool emit_sequence_item(Node *each, void *context);

struct emit_context
{
    mapping_iterator emit_mapping_item;
    bool wrap_collections;
};

typedef struct emit_context emit_context;

#define EMIT(STR) if(-1 == fprintf(stdout, (STR)))                      \
    {                                                                   \
        log_error("shell", "uh oh! couldn't emit literal %s", (STR));   \
        return false;                                                   \
    }    


