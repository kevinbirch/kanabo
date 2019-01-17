#include <stdio.h>
#include <yaml.h>

#include "emitter/yaml.h"
#include "log.h"

#define component "yaml"


static bool emit_node(Node *each, void *context);


static bool emit_document(Document *value, void *context)
{
    log_trace(component, "emitting document");
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    yaml_document_start_event_initialize(&event, &(yaml_version_directive_t){1, 1}, NULL, NULL, 0);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!emit_node(document_root(value), context))
    {
        return false;
    }

    yaml_document_end_event_initialize(&event, 0);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_sequence_item(Node *each, void *context)
{
    log_trace(component, "emitting sequence item");
    return emit_node(each, context);
}

static bool emit_sequence(Sequence *value, void *context)
{
    log_trace(component, "emitting seqence");
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    log_trace(component, "seqence start");
    uint8_t *name = node_name(value);
    yaml_char_t *tag = NULL == name ? (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG : (yaml_char_t *)name;
    yaml_sequence_start_event_initialize(&event, NULL, tag, NULL == name, YAML_BLOCK_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!sequence_iterate(value, emit_sequence_item, context))
    {
        return false;
    }

    log_trace(component, "seqence end");
    yaml_sequence_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_tagged_scalar(const String *value, yaml_char_t *tag, yaml_scalar_style_t style, int implicit, void *context)
{
    log_trace("emitting scalar \"%s\"", C(value));
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    yaml_scalar_event_initialize(&event, NULL, tag, (yaml_char_t *)C(value),
                                 (int)node_size(value), implicit, implicit, style);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_mapping_item(String *key, Node *value, void *context)
{
    log_trace(component, "emitting mapping item");
    if(!emit_tagged_scalar(key, (yaml_char_t *)YAML_STR_TAG, YAML_PLAIN_SCALAR_STYLE, 1, context))
    {
        return false;
    }
    return emit_node(value, context);
}

static bool emit_mapping(Mapping *value, void *context)
{
    log_trace(component, "emitting mapping");
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;
    yaml_event_t event;

    log_trace(component, "mapping start");
    uint8_t *name = node_name(value);
    yaml_char_t *tag = NULL == name ? (yaml_char_t *)YAML_DEFAULT_MAPPING_TAG : (yaml_char_t *)name;
    yaml_mapping_start_event_initialize(&event, NULL, tag, NULL == name, YAML_BLOCK_MAPPING_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!mapping_iterate(value, emit_mapping_item, context))
    {
        return false;
    }

    log_trace(component, "mapping end");
    yaml_mapping_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

static bool emit_scalar(const Scalar *each, void *context)
{
    yaml_char_t *tag = NULL;
    yaml_scalar_style_t style = YAML_PLAIN_SCALAR_STYLE;
    uint8_t *name = node_name(each);

    switch(scalar_kind(each))
    {
        case SCALAR_STRING:
            tag = NULL == name ? (yaml_char_t *)YAML_STR_TAG : name;
            style = YAML_DOUBLE_QUOTED_SCALAR_STYLE;
            break;
        case SCALAR_INTEGER:
            tag = NULL == name ? (yaml_char_t *)YAML_INT_TAG : name;
            break;
        case SCALAR_REAL:
            tag = NULL == name ? (yaml_char_t *)YAML_FLOAT_TAG : name;
            break;
        case SCALAR_TIMESTAMP:
            tag = NULL == name ? (yaml_char_t *)YAML_TIMESTAMP_TAG : name;
            break;
        case SCALAR_BOOLEAN:
            tag = NULL == name ? (yaml_char_t *)YAML_BOOL_TAG : name;
            break;
        case SCALAR_NULL:
            tag = NULL == name ? (yaml_char_t *)YAML_NULL_TAG : name;
            break;
    }

    return emit_tagged_scalar(scalar_value(each), tag, style, NULL == name, context);
}

static bool emit_node(Node *each, void *context)
{
    bool result = true;
    switch(node_kind(each))
    {
        case DOCUMENT:
            result = emit_document(document(each), context);
            break;
        case SCALAR:
            result = emit_scalar(scalar(each), context);
            break;
        case SEQUENCE:
            result = emit_sequence(sequence(each), context);
            break;
        case MAPPING:
            result = emit_mapping(mapping(each), context);
            break;
        case ALIAS:
            result = emit_node(alias_target(alias(each)), context);
            break;
    }

    return result;
}

static bool emit_nodelist(const Nodelist *list, yaml_emitter_t *emitter)
{
    log_trace(component, "emitting nodelist");
    yaml_event_t event;

    log_trace(component, "seqence start");
    yaml_sequence_start_event_initialize(&event, NULL, (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG, 1, YAML_BLOCK_SEQUENCE_STYLE);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    if(!nodelist_iterate(list, emit_sequence_item, emitter))
    {
        return false;
    }

    log_trace(component, "seqence end");
    yaml_sequence_end_event_initialize(&event);
    if (!yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool emit_yaml(const Nodelist *list)
{
    log_debug(component, "emitting...");
    yaml_emitter_t emitter;
    yaml_event_t event;
    bool result = true;

    yaml_emitter_initialize(&emitter);
    yaml_emitter_set_output_file(&emitter, stdout);
    yaml_emitter_set_unicode(&emitter, 1);

    log_trace(component, "stream start");
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    if (!yaml_emitter_emit(&emitter, &event))
    {
        result = false;
        goto end;
    }

    log_trace(component, "document start");
    yaml_document_start_event_initialize(&event, &(yaml_version_directive_t){1, 1}, NULL, NULL, 0);
    if (!yaml_emitter_emit(&emitter, &event))
    {
        result = false;
        goto end;
    }

    if(!emit_nodelist(list, &emitter))
    {
        result = false;
        goto end;
    }

    log_trace(component, "document end");
    yaml_document_end_event_initialize(&event, 1);
    if (!yaml_emitter_emit(&emitter, &event))
    {
        result = false;
        goto end;
    }

    log_trace(component, "stream end");
    yaml_stream_end_event_initialize(&event);
    if (!yaml_emitter_emit(&emitter, &event))
    {
        result = false;
    }

  end:
    yaml_emitter_delete(&emitter);
    return result;
}
