#include <yaml.h>

#include "emitter/yaml.h"
#include "log.h"
#include "panic.h"
#include "xalloc.h"

#define component "yaml"

static bool emit_node(Node *each, void *context);

static bool emit_scalar(const Scalar *each, yaml_emitter_t *emitter)
{
    yaml_char_t *anchor = (yaml_char_t *)C(each->base_yaml.anchor);
    yaml_char_t *tag = (yaml_char_t *)C(node_name(each));
    yaml_char_t *value = (yaml_char_t *)C(scalar_value(each));
    int length = (int)node_size(each);
    int implicit = 0 == memcmp("tag:yaml.org", tag, 12) ? 1 : 0;  // N.B. - emit custom tags
    yaml_scalar_style_t style = YAML_PLAIN_SCALAR_STYLE;

    switch(each->yaml.style)
    {
        case STYLE_PLAIN:
            style = YAML_PLAIN_SCALAR_STYLE;
            break;
        case STYLE_SINGLE_QUOTE:
            style = YAML_SINGLE_QUOTED_SCALAR_STYLE;
            break;
        case STYLE_DOUBLE_QUOTE:
            style = YAML_DOUBLE_QUOTED_SCALAR_STYLE;
            break;
        case STYLE_LITERAL:
            style = YAML_LITERAL_SCALAR_STYLE;
            break;
        case STYLE_FOLDED:
            style = YAML_FOLDED_SCALAR_STYLE;
            break;
    }

    yaml_event_t event;

    yaml_scalar_event_initialize(&event, anchor, tag, value, length, implicit, implicit, style);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    return true;
}

static bool emit_sequence_item(Node *each, void *context)
{
    return emit_node(each, context);
}

static bool emit_sequence(Sequence *value, yaml_emitter_t *emitter)
{
    if(sequence_is_empty(value))
    {
        return true;
    }

    yaml_char_t *anchor = (yaml_char_t *)C(value->base_yaml.anchor);
    yaml_char_t *tag = (yaml_char_t *)C(node_name(value));
    int implicit = 0 == memcmp("tag:yaml.org", tag, 12) ? 1 : 0;  // N.B. - emit custom tags
    yaml_sequence_style_t style = YAML_BLOCK_SEQUENCE_STYLE;

    if(STYLE_FLOW == value->yaml.style)
    {
        style = YAML_FLOW_SEQUENCE_STYLE;
    }

    yaml_event_t event;

    yaml_sequence_start_event_initialize(&event, anchor, tag, implicit, style);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    if(!sequence_iterate(value, emit_sequence_item, emitter))
    {
        return false;
    }

    yaml_sequence_end_event_initialize(&event);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    return true;
}

static bool emit_mapping_item(Scalar *key, Node *value, void *context)
{
    if(!emit_scalar(key, (yaml_emitter_t *)context))
    {
        return false;
    }

    return emit_node(value, context);
}

static bool emit_mapping(Mapping *value, yaml_emitter_t *emitter)
{
    if(mapping_is_empty(value))
    {
        return true;
    }

    yaml_char_t *anchor = (yaml_char_t *)C(value->base_yaml.anchor);
    yaml_char_t *tag = (yaml_char_t *)C(node_name(value));
    int implicit = 0 == memcmp("tag:yaml.org", tag, 12) ? 1 : 0;  // N.B. - emit custom tags
    yaml_mapping_style_t style = YAML_BLOCK_MAPPING_STYLE;

    if(STYLE_FLOW == value->yaml.style)
    {
        style = YAML_FLOW_MAPPING_STYLE;
    }
    
    yaml_event_t event;

    yaml_mapping_start_event_initialize(&event, anchor, tag, implicit, style);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    if(!mapping_iterate(value, emit_mapping_item, emitter))
    {
        return false;
    }

    yaml_mapping_end_event_initialize(&event);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    return true;
}

static bool emit_node(Node *each, void *context)
{
    yaml_emitter_t *emitter = (yaml_emitter_t *)context;

    switch(node_kind(each))
    {
        case DOCUMENT:
            log_trace(component, "emitting document");
            return emit_node(document_root(document(each)), emitter);
            break;
        case SCALAR:
            log_trace(component, "emitting scalar: \"%s\"", C(scalar_value(scalar(each))));
            return emit_scalar(scalar(each), emitter);
            break;
        case SEQUENCE:
            log_trace(component, "emitting seqence, len: %zu", node_size(each));
            return emit_sequence(sequence(each), emitter);
            break;
        case MAPPING:
            log_trace(component, "emitting mapping, len: %zu", node_size(each));
            return emit_mapping(mapping(each), emitter);
            break;
        case ALIAS:
            log_trace(component, "resolving alias");
            return emit_node(alias_target(alias(each)), context);
            break;
        default:
            return false;
    }
}

static bool emit_nodelist(const Nodelist *list, yaml_emitter_t *emitter)
{
    if(1 == nodelist_length(list))
    {
        return emit_node(nodelist_get(list, 0), emitter);
    }

    yaml_event_t event;

    yaml_sequence_start_event_initialize(&event, NULL, (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG, 1, YAML_BLOCK_SEQUENCE_STYLE);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    if(!nodelist_iterate(list, emit_sequence_item, emitter))
    {
        return false;
    }

    yaml_sequence_end_event_initialize(&event);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    return true;
}

static bool emit_document(const Nodelist *list, yaml_emitter_t *emitter)
{
    yaml_event_t event;

    int major = 1;
    int minor = 1;
    yaml_tag_directive_t *start = NULL;
    yaml_tag_directive_t *end = NULL;

    if(nodelist_is_empty(list))
    {
        goto emit;
    }

    Document *doc = ((Node *)nodelist_get(list, 0))->document;
    major = doc->yaml.major;
    minor = doc->yaml.minor;

    if(vector_is_empty(doc->yaml.tags))
    {
        goto emit;
    }

    size_t length = vector_length(doc->yaml.tags);
    start = xcalloc((sizeof(yaml_tag_directive_t)*length)+1);  // N.B. - NULL terminating entry
    yaml_tag_directive_t *cur = start;

    for(size_t i = 0; i < length; i++)
    {
        TagDirective *td = vector_get(doc->yaml.tags, i);
        cur->handle = (yaml_char_t *)strdup(td->handle);
        cur->prefix = (yaml_char_t *)strdup(td->prefix);
        cur++;
    }
    end = cur;

  emit:
    ;
    yaml_version_directive_t *version = &(yaml_version_directive_t){major, minor};
    yaml_document_start_event_initialize(&event, version, start, end, 0);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    if(!emit_nodelist(list, emitter))
    {
        return false;
    }

    yaml_document_end_event_initialize(&event, 1);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    return true;
}

static bool emit_stream(const Nodelist *list, yaml_emitter_t *emitter)
{
    yaml_event_t event;

    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    if(!emit_document(list, emitter))
    {
        return false;
    }

    yaml_stream_end_event_initialize(&event);
    if(!yaml_emitter_emit(emitter, &event))
    {
        return false;
    }

    return true;
}

bool emit_yaml(const Nodelist *list)
{
    log_debug(component, "emitting %zu items...", nodelist_length(list));
    yaml_emitter_t emitter;

    yaml_emitter_initialize(&emitter);
    yaml_emitter_set_output_file(&emitter, stdout);
    yaml_emitter_set_unicode(&emitter, 1);

    bool result = emit_stream(list, &emitter);

    yaml_emitter_delete(&emitter);
    fflush(stdout);

    return result;
}
