#include <stdio.h>

#include "emitter/bash.h"
#include "emitter/shell.h"
#include "log.h"

static bool emit_mapping_item(Node *key, Node *value, void *context);

bool emit_bash(const Nodelist *list)
{
    log_debug("bash", "emitting %zd items...", nodelist_length(list));
    emit_context context = {
            .emit_mapping_item = emit_mapping_item,
            .wrap_collections = true
    };

    return nodelist_iterate(list, emit_node, &context);
}

static bool emit_mapping_item(Node *key, Node *value, void *context)
{
    if(is_scalar(value))
    {
        log_trace("bash", "emitting mapping item");
        EMIT("[");
        log_trace("bash", "emitting mapping item key");
        if(!emit_raw_scalar(scalar(key)))
        {
            log_error("bash", "uh oh! couldn't emit mapping key");
            return false;
        }
        EMIT("]=");
        log_trace("bash", "emitting mapping item value");
        if(!emit_scalar(scalar(value)))
        {
            log_error("bash", "uh oh! couldn't emit mapping value");
            return false;
        }
        EMIT(" ");
    }
    else
    {
        log_trace("bash", "skipping mapping item");
    }

    return true;
}
