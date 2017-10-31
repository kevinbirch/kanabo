#pragma once

#include <stdbool.h>

#include "document.h"

bool emit_node(node *value, void *context);
bool emit_scalar(const node *each);
bool emit_quoted_scalar(const node *each);
bool emit_raw_scalar(const node *each);
bool emit_sequence_item(node *each, void *context);

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


