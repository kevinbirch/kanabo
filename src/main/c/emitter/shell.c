#include <ctype.h>

#include "emitter/emit.h"
#include "emitter/shell.h"

#define component "shell"

#define MAYBE_EMIT(STR)                                 \
    if(context->wrap_collections)                       \
    {                                                   \
        EMIT((STR));                                    \
    }

static bool contains_space(const String *value)
{
    const uint8_t *raw = strdta(value);
    for(size_t i = 0; i < strlen(value); i++)
    {
        if(isspace(*(raw + i)))
        {
            return true;
        }
    }

    return false;
}

bool emit_quoted_string(const String *value)
{
    EMIT("'");
    if(!emit_string(value))
    {
        return false;
    }
    EMIT("'");

    return true;
}

bool emit_scalar(const Scalar *each)
{
    String *value = scalar_value(each);
    if(SCALAR_STRING == scalar_kind(each) && contains_space(value))
    {
        return emit_quoted_string(value);
    }

    return emit_string(value);
}

bool emit_sequence_item(Node *each, void *context)
{
    if(is_scalar(each))
    {
        log_debug(component, "skipping non-scalar sequence item value: %s", node_kind_name(each));
        return true;
    }

    Scalar *value = scalar(each);
    if(!emit_scalar(value))
    {
        return false;
    }
    EMIT(" ");

    return true;
}

bool emit_node(Node *each, void *argument)
{
    emit_context *context = (emit_context *)argument;
    bool result = true;

    switch(node_kind(each))
    {
        case DOCUMENT:
            result = emit_node(document_root(document(each)), NULL);
            break;
        case SCALAR:
            result = emit_scalar(scalar(each));
            EMIT("\n");
            break;
        case SEQUENCE:
            MAYBE_EMIT("(");
            result = sequence_iterate(sequence(each), emit_sequence_item, NULL);
            MAYBE_EMIT(")");
            EMIT("\n");
            break;
        case MAPPING:
            MAYBE_EMIT("(");
            result = mapping_iterate(mapping(each), context->emit_mapping_item, NULL);
            MAYBE_EMIT(")");
            EMIT("\n");
            break;
        case ALIAS:
            result = emit_node(alias_target(alias(each)), NULL);
            break;
    }

    return result;
}
