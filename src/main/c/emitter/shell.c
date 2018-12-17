#include <ctype.h>
#include <stdio.h>

#include "emitter/shell.h"
#include "log.h"


#define MAYBE_EMIT(STR) if(context->wrap_collections)   \
    {                                                   \
        EMIT((STR));                                    \
    }

bool emit_node(Node *each, void *argument)
{
    emit_context *context = (emit_context *)argument;

    log_debug("shell", "emitting node...");
    bool result = true;
    switch(node_kind(each))
    {
        case DOCUMENT:
            log_trace("shell", "emitting document");
            result = emit_node(document_root(document(each)), NULL);
            break;
        case SCALAR:
            result = emit_scalar(scalar(each));
            EMIT("\n");
            break;
        case SEQUENCE:
            log_trace("shell", "emitting seqence");
            MAYBE_EMIT("(");
            result = sequence_iterate(sequence(each), emit_sequence_item, NULL);
            MAYBE_EMIT(")");
            EMIT("\n");
            break;
        case MAPPING:
            log_trace("shell", "emitting mapping");
            MAYBE_EMIT("(");
            result = mapping_iterate(mapping(each), context->emit_mapping_item, NULL);
            MAYBE_EMIT(")");
            EMIT("\n");
            break;
        case ALIAS:
            log_trace("shell", "resolving alias");
            result = emit_node(alias_target(alias(each)), NULL);
            break;
    }

    return result;
}

static bool scalar_contains_space(const Scalar *each)
{
    uint8_t *value = scalar_value(each);
    for(size_t i = 0; i < node_size(each); i++)
    {
        if(isspace(*(value + i)))
        {
            return true;
        }
    }

    return false;
}

bool emit_scalar(const Scalar *each)
{
    if(SCALAR_STRING == scalar_kind(each) && scalar_contains_space(each))
    {
        log_trace("shell", "emitting quoted scalar");
        return emit_quoted_scalar(each);
    }
    else
    {
        log_trace("shell", "emitting raw scalar");
        return emit_raw_scalar(each);
    }
}

bool emit_quoted_scalar(const Scalar *each)
{
    EMIT("'");
    if(!emit_raw_scalar(each))
    {
        log_error("shell", "uh oh! couldn't emit quoted scalar");
        return false;
    }
    EMIT("'");

    return true;
}

bool emit_raw_scalar(const Scalar *each)
{
    return 1 == fwrite(scalar_value(each), node_size(each), 1, stdout);
}

bool emit_sequence_item(Node *each, void *context)
{
    if(is_scalar(each))
    {
        log_trace("shell", "emitting sequence item");
        if(!emit_scalar(scalar(each)))
        {
            return false;
        }
        EMIT(" ");
    }
    else
    {
        log_trace("shell", "skipping sequence item");
    }

    return true;
}
