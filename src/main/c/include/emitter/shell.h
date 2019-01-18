#pragma once

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "document.h"
#include "emitter/emit.h"

bool emit_node(Node *value, void *context);
bool emit_scalar(const Scalar *);
bool emit_quoted_string(const String *);
bool emit_sequence_item(Node *each, void *context);

struct emit_context
{
    mapping_iterator emit_mapping_item;
    bool wrap_collections;
};

typedef struct emit_context emit_context;
