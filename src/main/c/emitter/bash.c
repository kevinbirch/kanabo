#include "emitter/bash.h"
#include "emitter/shell.h"

#define component "bash"

static bool emit_mapping_item(Scalar *key, Node *each, void *context)
{
    if(!is_scalar(each))
    {
        log_trace(component, "skipping non-scalar mapping item value: %s", node_kind_name(each));
        return true;
    }

    EMIT("[");
    if(!emit_string(scalar_value(key)))
    {
        return false;
    }
    EMIT("]=");

    Scalar *value = scalar(each);
    log_trace(component, "emitting scalar: \"%s\"", C(scalar_value(value)));
    if(!emit_scalar(value))
    {
        return false;
    }
    EMIT(" ");

    return true;
}

bool emit_bash(const Nodelist *list)
{
    log_debug(component, "emitting %zu items...", nodelist_length(list));
    emit_context context = {
            .emit_mapping_item = emit_mapping_item,
            .wrap_collections = true
    };

    bool result = nodelist_iterate(list, emit_node, &context);
    fflush(stdout);

    return result;
}
