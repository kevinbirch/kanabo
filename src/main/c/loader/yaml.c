#include <errno.h>
#include <regex.h>
#include <stdlib.h>
#include <stdbool.h>

#include <yaml.h>

#include "loader.h"
#include "loader/debug.h"
#include "loader/error.h"
#include "panic.h"
#include "str.h"
#include "vector.h"
#include "xalloc.h"

static const char * const INTEGER_PATTERN = "^(0|([+-]?[1-9][[:digit:]]*))$";
static const char * const DECIMAL_PATTERN = "^(0|([+-]?[1-9][[:digit:]]*))?\\.[[:digit:]]+([eE][+-]?[[:digit:]]+)?$";
static const char * const TIMESTAMP_PATTERN = "^[0-9][0-9][0-9][0-9]-[0-9][0-9]?-[0-9][0-9]?(([Tt]|[ \t]+)[0-9][0-9]?:[0-9][0-9](:[0-9][0-9])?([.][0-9]+)?([ \t]*(Z|([-+][0-9][0-9]?(:[0-9][0-9])?)))?)?$";

static regex_t decimal_regex;
static regex_t integer_regex;
static regex_t timestamp_regex;

static const Position NO_POSITION = (Position){.index=0, .line=0, .offset=0};

typedef DuplicateKeyStrategy Strategy;

struct loader_context_s
{
    Strategy      strategy;
    const String *input_name;
    DocumentSet  *documents;
    Vector       *errors;
    Vector       *detached;
    Node         *current;
    Node         *key_cache;
    bool          fatal_error;
};

typedef struct loader_context_s Loader;

#define current_document(CONTEXT) vector_last((CONTEXT)->documents->values)
#define position(MARK) (Position){.index=(MARK).index, .line=(MARK).line, .offset=(MARK).column}

static void must_make_regex(regex_t *regex, const char * pattern)
{
    int code = regcomp(regex, pattern, REG_EXTENDED | REG_NOSUB);
    if(0 != code)
    {
        char buf[256];
        regerror(code, regex, buf, sizeof(buf));

        panicf("loader: initialize: regex compilation failed: \"%s\": %s", pattern, buf);
    }       
}

static void context_init(Loader *context, DuplicateKeyStrategy strategy, const String *input_name)
{
    context->errors = make_vector_with_capacity(1);
    context->detached = make_vector_with_capacity(1);
    context->documents = make_document_set();
    context->strategy = strategy;
    context->input_name = input_name;
    context->fatal_error = false;
}

static inline void detach(Loader *context, Node *node)
{
    loader_tracef("detaching node (%p)", (void *)node);
    vector_append(context->detached, node);
}

static void interpret_yaml_error(Loader *context, yaml_parser_t *parser)
{
    LoaderErrorCode code = 0;

    switch (parser->error)
    {
        case YAML_READER_ERROR:
            code = ERR_READER_FAILED;
            break;
        case YAML_SCANNER_ERROR:
            code = ERR_SCANNER_FAILED;
            break;
         case YAML_PARSER_ERROR:
            code = ERR_PARSER_FAILED;
            break;
        default:
            code = ERR_INTERNAL_LIBYAML;
            break;
    }

    Position pos = position(parser->problem_mark);
    if(NULL != parser->context)
    {
        pos = position(parser->context_mark);
    }

    String *extra = NULL;
    if(NULL == parser->context)
    {
        extra = make_string(parser->problem);
    }
    else
    {
        extra = format("%s %s", parser->problem, parser->context);
        if(NULL == extra)
        {
            extra = make_string(parser->problem);
        }
    }
    add_loader_error_with_extra(context->errors, pos, code, extra);
    context->fatal_error = true;
}

static void set_anchor(Loader *context, Node *target, uint8_t *value)
{
    if(NULL == value)
    {
        return;
    }

    document_track_anchor(current_document(context), value, target);
}

static void add_to_mapping(Loader *context, Node *node)
{
    node->parent = context->current;  // N.B. - all nodes need a parent since they may be a container

    if(NULL == context->key_cache)  // N.B. - node is a mapping key
    {
        loader_tracef("caching node (%p) as key for mapping context (%p)", (void *)node, (void *)context->current);
        context->key_cache = node;

        return;
    }

    if(!is_scalar(context->key_cache))
    {
        loader_debug("uh oh! found a non scalar mapping key");
        String *extra = make_string(node_kind_name(context->key_cache));
        Position pos = context->key_cache->position;
        add_loader_error_with_extra(context->errors, pos, ERR_NON_SCALAR_KEY, extra);

        loader_trace("detaching value for non-scalar key");
        detach(context, context->key_cache);  // N.B. - key is not needed, mapping is abandoned
        detach(context, node);  // N.B. - value not needed, mapping is abandoned

        goto cleanup;
    }

    Scalar *key = scalar(context->key_cache);

    Mapping *mapping = mapping(context->current);
    Node *previous = mapping_lookup(mapping, scalar_value(key));

    if(NULL != previous)
    {
        loader_debug("uh oh! found a duplicate key");
        detach(context, context->key_cache);  // N.B. - key is not needed, already exists

        if(DUPE_FAIL == context->strategy)
        {
            String *extra = strcln(scalar_value(key));
            Position pos = key->position;
            add_loader_error_with_extra(context->errors, pos, ERR_DUPLICATE_KEY, extra);
            detach(context, node);  // N.B. - value not needed, mapping is abandoned

            goto cleanup;
        }

        detach(context, previous);  // N.B. - previous value is not needed, it will be replaced
    }

    mapping_put(mapping, key, node);

  cleanup:
    context->key_cache = NULL;
}

static void add_node(Loader *context, Node *node, const yaml_event_t *event)
{
    switch(node_kind(context->current))
    {
        case DOCUMENT:
            loader_tracef("adding node (%p) to document context (%p)", (void *)node, (void *)context->current);
            document_set_root(document(context->current), node);
            break;
        case SEQUENCE:
            loader_tracef("adding node (%p) to sequence context (%p)", (void *)node, (void *)context->current);
            sequence_add(sequence(context->current), node);
            break;
        case MAPPING:
            loader_tracef("adding node (%p) to mapping context (%p)", (void *)node, (void *)context->current);
            add_to_mapping(context, node);
            break;
        default:
            loader_debugf("uh oh! an unsupported node kind has become the context node: %s", node_kind_name(context->current));
            String *extra = make_string(node_kind_name(node));
            Position pos = position(event->start_mark);
            add_loader_error_with_extra(context->errors, pos, ERR_INTERNAL_CTX_NODE, extra);
            context->fatal_error = true;
            loader_trace("detaching unsupported context node");
            detach(context, node);
            break;
    }
}

static void start_document(Loader *context, const yaml_event_t *event)
{
    Document *document = make_document_node();
    document->position = position(event->start_mark);

    document->yaml.major = 1;
    document->yaml.minor = 1;
    yaml_version_directive_t *version = event->data.document_start.version_directive;
    if(NULL != version)
    {
        document->yaml.major = version->major;
        document->yaml.minor = version->minor;
    }

    yaml_tag_directive_t *start = event->data.document_start.tag_directives.start;
    yaml_tag_directive_t *end = event->data.document_start.tag_directives.end;
    if(NULL == start)
    {
        goto set_context;
    }

    Vector *tags = make_vector_with_capacity(1);
    yaml_tag_directive_t *cur = start;
    while(cur != end)
    {
        TagDirective *td = xcalloc(sizeof(TagDirective));
        td->handle = make_string((const char *)cur->handle);
        td->prefix = make_string((const char *)cur->prefix);
        vector_add(tags, td);
        cur++;
    }
    document->yaml.tags = tags;
    
  set_context:
    context->current = node(document);
    document_set_add(context->documents, document);

    loader_tracef("started document (%p)", (void *)document);
}

static void end_document(Loader *context)
{
    loader_tracef("completed document (%p)", (void *)context->current);

    context->current = NULL;
}

static void start_sequence(Loader *context, const yaml_event_t *event)
{
    Sequence *sequence = make_sequence_node();
    sequence->position = position(event->start_mark);

    switch(event->data.sequence_start.style)
    {
        case YAML_BLOCK_SEQUENCE_STYLE:
            sequence->yaml.style = STYLE_BLOCK;
            break;
        case YAML_FLOW_SEQUENCE_STYLE:
            sequence->yaml.style = STYLE_FLOW;
            break;
        default:
            sequence->yaml.style = STYLE_BLOCK;
            break;
    }

    if(NULL != event->data.sequence_start.tag)
    {
        loader_debug("found event tag adding sequence");
        node_set_tag(sequence, make_string((const char *)event->data.sequence_start.tag));
    }
    else
    {
        node_set_tag(sequence, make_string(YAML_DEFAULT_SEQUENCE_TAG));
    }
    set_anchor(context, node(sequence), event->data.sequence_start.anchor);

    add_node(context, node(sequence), event);
    context->current = node(sequence);

    loader_tracef("started sequence (%p)", (void *)sequence);
}

static void end_sequence(Loader *context)
{
    Sequence *sequence = sequence(context->current);
    vector_trim(sequence->values);
    context->current = sequence->parent;

    loader_tracef("completed sequence (%p) of length: %zu", (void *)sequence, node_size(sequence));
}

static void start_mapping(Loader *context, const yaml_event_t *event)
{
    Mapping *mapping = make_mapping_node();
    mapping->position = position(event->start_mark);

    switch(event->data.mapping_start.style)
    {
        case YAML_BLOCK_MAPPING_STYLE:
            mapping->yaml.style = STYLE_BLOCK;
            break;
        case YAML_FLOW_MAPPING_STYLE:
            mapping->yaml.style = STYLE_FLOW;
            break;
        default:
            mapping->yaml.style = STYLE_BLOCK;
            break;
    }

    if(NULL != event->data.mapping_start.tag)
    {
        loader_debug("found event tag adding mapping");
        node_set_tag(mapping, make_string((const char *)event->data.mapping_start.tag));
    }
    else
    {
        node_set_tag(mapping, make_string(YAML_DEFAULT_MAPPING_TAG));
    }
    set_anchor(context, node(mapping), event->data.mapping_start.anchor);

    add_node(context, node(mapping), event);
    context->current = node(mapping);

    loader_tracef("started mapping (%p)", (void *)mapping);
}

static void end_mapping(Loader *context)
{
    Mapping *mapping = mapping(context->current);
    context->current = mapping->parent;

    loader_tracef("completed mapping (%p) of length: %zu", (void *)mapping, node_size(mapping));
}

static void add_alias(Loader *context, const yaml_event_t *event)
{
    Node *target = document_resolve_anchor(current_document(context), event->data.alias.anchor);
    if(NULL == target)
    {
        loader_debug("uh oh! alias anchor is not known");
        String *extra = make_string((char *)event->data.alias.anchor);
        Position pos = position(event->start_mark);
        add_loader_error_with_extra(context->errors, pos, ERR_NO_ANCHOR_FOR_ALIAS, extra);
        return;
    }

    for(Node *cur = node_parent(context->current); NULL != cur; cur = node_parent(cur))
    {
        if(cur == target)
        {
            loader_debug("uh oh! an alias loop was detected");
            String *extra = make_string((char *)event->data.alias.anchor);
            Position pos = position(event->start_mark);
            add_loader_error_with_extra(context->errors, pos, ERR_ALIAS_LOOP, extra);
            return;
        }
    }

    String *anchor = make_string((const char *)event->data.alias.anchor);
    Alias *alias = make_alias_node(target, anchor);
    alias->position = position(event->start_mark);

    add_node(context, node(alias), event);

    loader_tracef("added alias \"%s\" for target (%p)", event->data.alias.anchor, (void *)target);
}

static bool match_decimal(const char *value)
{
    static bool init = false;
    if(!init)
    {
        must_make_regex(&decimal_regex, DECIMAL_PATTERN);
        init = true;
    }

    return 0 == regexec(&decimal_regex, value, 0, NULL, 0);
}

static bool match_integer(const char *value)
{
    static bool init = false;
    if(!init)
    {
        must_make_regex(&integer_regex, INTEGER_PATTERN);
        init = true;
    }

    return 0 == regexec(&integer_regex, value, 0, NULL, 0);
}

static bool match_timestamp(const char *value)
{
    static bool init = false;
    if(!init)
    {
        must_make_regex(&timestamp_regex, TIMESTAMP_PATTERN);
        init = true;
    }

    return 0 == regexec(&timestamp_regex, value, 0, NULL, 0);
}

static Scalar *build_scalar(Loader *context, const yaml_event_t *event)
{
    String *value = make_string_with_bytestring(event->data.scalar.value, event->data.scalar.length);
    Scalar *scalar = make_scalar_node(value, SCALAR_STRING);
    scalar->position = position(event->start_mark);

    switch(event->data.scalar.style)
    {
        case YAML_SINGLE_QUOTED_SCALAR_STYLE:
            scalar->yaml.style = STYLE_SINGLE_QUOTE;
            break;
        case YAML_DOUBLE_QUOTED_SCALAR_STYLE:
            scalar->yaml.style = STYLE_DOUBLE_QUOTE;
            break;
        case YAML_LITERAL_SCALAR_STYLE:
            scalar->yaml.style = STYLE_LITERAL;
            break;
        case YAML_FOLDED_SCALAR_STYLE:
            scalar->yaml.style = STYLE_FOLDED;
            break;
        default:
            scalar->yaml.style = STYLE_PLAIN;
            break;
    }

    const char *yaml_tag = YAML_STR_TAG;

    if(0 == memcmp("null", event->data.scalar.value, 4))
    {
        loader_trace("found scalar null");
        scalar->kind = SCALAR_NULL;
        yaml_tag = YAML_NULL_TAG;
    }
    else if(0 == memcmp("true", event->data.scalar.value, 4))
    {
        loader_tracef("found scalar boolean \"%s\"", event->data.scalar.value);
        scalar->kind = SCALAR_BOOLEAN;

        scalar->boolean = true;
        yaml_tag = YAML_BOOL_TAG;
    }
    else if(0 == memcmp("false", event->data.scalar.value, 5))
    {
        loader_tracef("found scalar boolean \"%s\"", event->data.scalar.value);
        scalar->kind = SCALAR_BOOLEAN;

        scalar->boolean = false;
        yaml_tag = YAML_BOOL_TAG;
    }
    else if(match_decimal((const char *)event->data.scalar.value))
    {
        loader_tracef("found scalar real \"%s\"", event->data.scalar.value);
        scalar->kind = SCALAR_REAL;

        errno = 0;
        scalar->real = strtod((const char *)event->data.scalar.value, NULL);
        if(ERANGE == errno)
        {
            String *extra = make_string((const char *)event->data.scalar.value);
            Position pos = scalar->position;
            add_loader_error_with_extra(context->errors, pos, ERR_NUMBER_OUT_OF_RANGE, extra);
            scalar->real = 0.0;
        }

        yaml_tag = YAML_FLOAT_TAG;
    }
    else if(match_integer((const char *)event->data.scalar.value))
    {
        loader_tracef("found scalar integer \"%s\"", event->data.scalar.value);
        scalar->kind = SCALAR_INTEGER;

        errno = 0;
        scalar->integer = strtoll((const char *)event->data.scalar.value, NULL, 10);
        if(ERANGE == errno)
        {
            String *extra = make_string((const char *)event->data.scalar.value);
            Position pos = scalar->position;
            add_loader_error_with_extra(context->errors, pos, ERR_NUMBER_OUT_OF_RANGE, extra);
            scalar->integer = 0;
        }

        yaml_tag = YAML_INT_TAG;
    }
    else if(match_timestamp((const char *)event->data.scalar.value))
    {
        loader_tracef("found scalar timestamp \"%s\"", event->data.scalar.value);
        scalar->kind = SCALAR_TIMESTAMP;
        yaml_tag = YAML_TIMESTAMP_TAG;
    }

    if(NULL != event->data.scalar.tag)
    {
        node_set_tag(scalar, make_string((const char *)event->data.scalar.tag));
    }
    else
    {
        node_set_tag(scalar, make_string(yaml_tag));
    }

    return scalar;
}

static void add_scalar(Loader *context, const yaml_event_t *event)
{
    Scalar *scalar = build_scalar(context, event);
    set_anchor(context, node(scalar), event->data.scalar.anchor);

    add_node(context, node(scalar), event);

    loader_tracef("added scalar (%p)", (void *)scalar);
}

static bool dispatch_event(Loader *context, yaml_event_t *event)
{
    switch(event->type)
    {
        case YAML_NO_EVENT:
            loader_trace("noop event");
            break;

        case YAML_STREAM_START_EVENT:
            loader_trace("stream start event");
            break;

        case YAML_STREAM_END_EVENT:
            loader_trace("stream end event");
            return false;
            break;

        case YAML_DOCUMENT_START_EVENT:
            loader_trace("document start event");
            start_document(context, event);
            break;

        case YAML_DOCUMENT_END_EVENT:
            loader_trace("document end event");
            end_document(context);
            break;

        case YAML_SEQUENCE_START_EVENT:
            loader_trace("sequence start event");
            start_sequence(context, event);
            break;

        case YAML_SEQUENCE_END_EVENT:
            loader_trace("sequence end event");
            end_sequence(context);
            break;

        case YAML_MAPPING_START_EVENT:
            loader_trace("mapping start event");
            start_mapping(context, event);
            break;

        case YAML_MAPPING_END_EVENT:
            loader_trace("mapping end event");
            end_mapping(context);
            break;

        case YAML_ALIAS_EVENT:
            loader_trace("alias event");
            add_alias(context, event);
            break;

        case YAML_SCALAR_EVENT:
            loader_trace("scalar event");
            add_scalar(context, event);
            break;
    }

    yaml_event_delete(event);

    if(context->fatal_error)
    {
        return false;
    }

    return true;
}

static void event_loop(Loader *context, yaml_parser_t *parser)
{
    yaml_event_t event;

    while(1)
    {
        if(!yaml_parser_parse(parser, &event))
        {
            yaml_event_delete(&event);
            interpret_yaml_error(context, parser);
            break;
        }
        if(!dispatch_event(context, &event))
        {
            break;
        }
    }
}

static void node_destroyer(void *each)
{
    dispose_node(each);
}

static Maybe(DocumentSet) parse(const String *input_name, yaml_parser_t *parser, DuplicateKeyStrategy strategy)
{
    Loader context;
    memset(&context, 0, sizeof(Loader));

    context_init(&context, strategy, input_name);

    event_loop(&context, parser);

    if(NULL != context.key_cache)  // N.B. if the final mapping value is rejected...
    {
        loader_trace("detaching unused cached mapping key");
        detach(&context, context.key_cache);
        context.key_cache = NULL;
    }

    vector_destroy(context.detached, node_destroyer);

    if(vector_is_empty(context.errors))
    {
        loader_debugf("found %zu documents.", document_set_size(context.documents));

        if(0 == document_set_size(context.documents))
        {
            add_loader_error(context.errors, NO_POSITION, ERR_NO_DOCUMENTS_FOUND);
            goto failure;
        }

        dispose_vector(context.errors);
        context.documents->input_name = string_clone(input_name);
        return just(DocumentSet, context.documents);
    }

  failure:
    dispose_document_set(context.documents);
    return fail(DocumentSet, context.errors);
}

Maybe(DocumentSet) load_yaml_from_stdin(DuplicateKeyStrategy strategy)
{
    yaml_parser_t parser;
    if(!yaml_parser_initialize(&parser))
    {
        panic("loader: initialize: allocate yaml parser");
    }

    loader_debug("loading yaml from stdin");
    yaml_parser_set_input_file(&parser, stdin);

    String *name = S("stdin");
    Maybe(DocumentSet) result = parse(name, &parser, strategy);
    dispose_string(name);

    yaml_parser_delete(&parser);

    return result;
}

Maybe(DocumentSet) load_yaml(Input *input, DuplicateKeyStrategy strategy)
{
    if(NULL == input)
    {
        Vector *errors = make_vector_with_capacity(1);
        add_loader_error(errors, NO_POSITION, ERR_INPUT_IS_NULL);
        return fail(DocumentSet, errors);
    }

    yaml_parser_t parser;
    if(!yaml_parser_initialize(&parser))
    {
        panic("loader: initialize: allocate yaml parser");
    }

    loader_debugf("loading yaml from \"%s\"", C(input_name(input)));
    yaml_parser_set_input_string(&parser, (const unsigned char *)input->buffer, input_length(input));

    Maybe(DocumentSet) result = parse(input_name(input), &parser, strategy);

    yaml_parser_delete(&parser);

    return result;
}
