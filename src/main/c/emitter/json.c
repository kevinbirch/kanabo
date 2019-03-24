#include "emitter/emit.h"
#include "emitter/json.h"

#define component "json"

static bool emit_node(Node *each, void *context __attribute__((unused)));

static bool emit_quoted_scalar(const String *value)
{
    EMIT("\"");
    if(!emit_string(value))
    {
        return false;
    }
    EMIT("\"");

    return true;
}

static bool emit_scalar(const Scalar *each)
{
    if(SCALAR_STRING == scalar_kind(each) || SCALAR_TIMESTAMP == scalar_kind(each))
    {
        return emit_quoted_scalar(scalar_value(each));
    }

    return emit_string(scalar_value(each));
}

static bool emit_mapping_item(String *key, Node *value, void *context)
{
    size_t *count = (size_t *)context;
    if(0 != (*count)++)
    {
        EMIT(",");
    }

    if(!emit_quoted_scalar(key))
    {
        return false;
    }
    EMIT(":");

    return emit_node(value, NULL);
}

static bool emit_sequence_item(Node *each, void *context)
{
    size_t *count = (size_t *)context;
    if(0 != (*count)++)
    {
        EMIT(",");
    }

    return emit_node(each, NULL);
}

static bool emit_node(Node *each, void *context __attribute__((unused)))
{
    bool result = true;
    size_t sequence_count = 0;
    size_t mapping_count = 0;

    switch(node_kind(each))
    {
        case DOCUMENT:
            log_trace(component, "emitting document");
            result = emit_node(document_root(document(each)), context);
            break;
        case SCALAR:
            log_trace(component, "emitting scalar: \"%s\"", C(scalar_value(scalar(each))));
            result = emit_scalar(scalar(each));
            break;
        case SEQUENCE:
            log_trace(component, "emitting seqence, len: %zu", node_size(each));
            EMIT("[");
            result = sequence_iterate(sequence(each), emit_sequence_item, &sequence_count);
            EMIT("]");
            break;
        case MAPPING:
            log_trace(component, "emitting mapping, len: %zu", node_size(each));
            EMIT("{");
            result = mapping_iterate(mapping(each), emit_mapping_item, &mapping_count);
            EMIT("}");
            break;
        case ALIAS:
            log_trace(component, "resolving alias");
            result = emit_node(alias_target(alias(each)), context);
            break;
    }

    return result;
}

bool emit_json(const Nodelist *list)
{
    log_debug(component, "emitting %zu items...", nodelist_length(list));
    size_t count = 0;

    if(1 == nodelist_length(list))
    {
        return emit_node(nodelist_get(list, 0), NULL);
    }

    EMIT("[");
    bool result = nodelist_iterate(list, emit_sequence_item, &count);
    EMIT("]\n");
    fflush(stdout);

    return result;
}
