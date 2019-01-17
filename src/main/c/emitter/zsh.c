#include <stdio.h>

#include "emitter/zsh.h"
#include "emitter/shell.h"
#include "log.h"

static bool emit_mapping_item(String *key, Node *value, void * context)
{
    if(is_scalar(value))
    {
        log_trace("zsh", "emitting mapping item");
        if(!emit_quoted_string(key))
        {
            log_error("zsh", "uh oh! couldn't emit mapping key");
            return false;
        }
        EMIT(" ");
        if(!emit_scalar(scalar(value)))
        {
            log_error("zsh", "uh oh! couldn't emit mapping value");
            return false;
        }
        EMIT(" ");
    }
    else
    {
        log_trace("zsh", "skipping mapping item");
    }

    return true;
}

bool emit_zsh(const Nodelist *list)
{
    log_debug("zsh", "emitting...");
    emit_context context =
        {
            .emit_mapping_item = emit_mapping_item,
            .wrap_collections = false
        };

    return nodelist_iterate(list, emit_node, &context);
}
