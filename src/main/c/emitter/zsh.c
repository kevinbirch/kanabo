#include <stdio.h>

#include "emitter/shell.h"
#include "emitter/zsh.h"

#define component "zsh"

static bool emit_mapping_item(String *key, Node *each, void *context)
{
    if(!is_scalar(each))
    {
        log_trace("zsh", "skipping non-scalar mapping item value: %s", node_kind_name(each));
        return true;
    }

    if(!emit_quoted_string(key))
    {
        return false;
    }
    EMIT(" ");

    Scalar *value = scalar(each);
    log_trace("shell", "emitting scalar: \"%s\"", C(scalar_value(value)));
    if(!emit_scalar(value))
    {
        return false;
    }
    EMIT(" ");

    return true;
}

bool emit_zsh(const Nodelist *list)
{
    log_debug("zsh", "emitting %zd items...", nodelist_length(list));
    emit_context context =
        {
            .emit_mapping_item = emit_mapping_item,
            .wrap_collections = false
        };

    return nodelist_iterate(list, emit_node, &context);
}
