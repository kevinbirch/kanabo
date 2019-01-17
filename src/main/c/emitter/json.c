#include <stdio.h>

#include "emitter/json.h"
#include "emitter/scalar.h"
#include "log.h"

#define component "json"

#define EMIT(STR) if(EOF == fputs((STR), stdout))                       \
    {                                                                   \
        log_error(component, "uh oh! couldn't emit literal %s", (STR)); \
        return false;                                                   \
    }

#define QEMIT(STR) if(EOF == fputs((STR), stdout))                      \
    {                                                                   \
        log_error(component, "uh oh! couldn't emit literal %s", (STR)); \
    }


static bool emit_json_node(Node *each, void *context __attribute__((unused)));


static bool emit_json_sequence_item(Node *each, void *context)
{
    log_trace(component, "emitting sequence item");
    size_t *count = (size_t *)context;
    if(0 != (*count)++)
    {
        EMIT(",");
    }
    return emit_json_node(each, NULL);
}

bool emit_json(const Nodelist *list)
{
    log_debug(component, "emitting...");
    size_t count = 0;
    QEMIT("[");
    bool result = nodelist_iterate(list, emit_json_sequence_item, &count);
    QEMIT("]");
    QEMIT("\n");

    return result;
}

static bool emit_json_quoted_scalar(const String *value)
{
    EMIT("\"");
    if(!emit_raw_string(value))
    {
        log_error(component, "uh oh! couldn't emit quoted scalar");
        return false;
    }
    EMIT("\"");

    return true;
}

static bool emit_json_scalar(const Scalar *each)
{
    if(SCALAR_STRING == scalar_kind(each) ||
       SCALAR_TIMESTAMP == scalar_kind(each))
    {
        log_trace(component, "emitting quoted scalar");
        return emit_json_quoted_scalar(scalar_value(each));
    }
    else
    {
        log_trace(component, "emitting raw scalar");
        return emit_raw_string(scalar_value(each));
    }
}

static bool emit_json_mapping_item(String *key, Node *value, void *context)
{
    log_trace(component, "emitting mapping item");
    size_t *count = (size_t *)context;
    if(0 != (*count)++)
    {
        EMIT(",");
    }
    if(!emit_json_quoted_scalar(key))
    {
        return false;
    }
    EMIT(":");
    return emit_json_node(value, NULL);
}

static bool emit_json_node(Node *each, void *context __attribute__((unused)))
{
    bool result = true;
    size_t sequence_count = 0;
    size_t mapping_count = 0;
    switch(node_kind(each))
    {
        case DOCUMENT:
            log_trace(component, "emitting document");
            result = emit_json_node(document_root(document(each)), context);
            break;
        case SCALAR:
            result = emit_json_scalar(scalar(each));
            break;
        case SEQUENCE:
            log_trace(component, "emitting seqence");
            EMIT("[");
            result = sequence_iterate(sequence(each), emit_json_sequence_item, &sequence_count);
            EMIT("]");
            break;
        case MAPPING:
            log_trace(component, "emitting mapping");
            EMIT("{");
            result = mapping_iterate(mapping(each), emit_json_mapping_item, &mapping_count);
            EMIT("}");
            break;
        case ALIAS:
            log_trace(component, "resolving alias");
            result = emit_json_node(alias_target(alias(each)), context);
            break;
    }

    return result;
}
