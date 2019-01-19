#include <stdlib.h>
#include <stdbool.h>
#include <regex.h>

#include <yaml.h>

#include "loader.h"
#include "loader/debug.h"
#include "loader/error.h"
#include "panic.h"
#include "str.h"
#include "vector.h"
#include "xalloc.h"

static const char * const DECIMAL_PATTERN = "^-?(0|([1-9][[:digit:]]*))([.][[:digit:]]+)?([eE][+-]?[[:digit:]]+)?$";
static const char * const INTEGER_PATTERN = "^-?(0|([1-9][[:digit:]]*))$";
static const char * const TIMESTAMP_PATTERN = "^[0-9][0-9][0-9][0-9]-[0-9][0-9]?-[0-9][0-9]?(([Tt]|[ \t]+)[0-9][0-9]?:[0-9][0-9](:[0-9][0-9])?([.][0-9]+)?([ \t]*(Z|([-+][0-9][0-9]?(:[0-9][0-9])?)))?)?$";

static regex_t decimal_regex;
static regex_t integer_regex;
static regex_t timestamp_regex;

static const char * const REGEX_PANIC_MSG = "loader: initialize: regex compilation failed: \"%s\": %s";

static const Position NO_POSITION = (Position){};

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
};

typedef struct loader_context_s Loader;

#define current_document(CONTEXT) vector_last((CONTEXT)->documents)
#define position(MARK) (Position){index: (MARK).index, line: (MARK).line, offset: (MARK).column}

static void must_make_regex(regex_t *regex, const char * pattern)
{
    int code = regcomp(regex, pattern, REG_EXTENDED | REG_NOSUB);
    if(0 != code)
    {
        char buf[256];
        regerror(code, regex, buf, sizeof(buf));

        size_t len = (size_t)snprintf(NULL, 0, REGEX_PANIC_MSG, pattern, buf);
        char *message = xcalloc(len);
        snprintf(message, len, REGEX_PANIC_MSG, pattern, buf);

        panic(message);
    }       
}

static void context_init(Loader *context, DuplicateKeyStrategy strategy, const String *input_name)
{
    context->errors = make_vector_with_capacity(1);
    if(NULL == context->errors)
    {
        panic("loader: initialize: allocate error list");
    }

    context->detached = make_vector_with_capacity(1);
    if(NULL == context->detached)
    {
        panic("loader: initialize: allocate detached node list");
    }

    context->documents = make_document_set();
    if(NULL == context->documents)
    {
        panic("loader: initialize: allocate document set");
    }

    context->strategy = strategy;
    context->input_name = input_name;
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

    const char *extra = parser->problem;
    Position pos = position(parser->problem_mark);

    if(NULL != parser->context)
    {
        extra = parser->context;
        pos = position(parser->context_mark);
    }

    size_t length = strlen(extra) + 1;
    LoaderError *err = xcalloc(sizeof(LoaderError) + length);
    err->code = code;
    err->position = pos;

    if(1 < length)
    {
        memcpy(err->extra, extra, length);
    }
    else
    {
        err->extra[0] = '\0';
    }

    vector_append(context->errors, err);
}

static void set_anchor(Loader *context, Node *target, uint8_t *anchor)
{
    if(NULL == anchor)
    {
        return;
    }

    Document *document = current_document(context);
    if(NULL == document->anchors)
    {
        document->anchors = make_hashtable_with_function(string_comparitor, fnv1a_string_hash);
        if(NULL == document->anchors)
        {
            panic("loader: initialize: allocate anchor hashtable");
        }
    }

    node_set_anchor(target, anchor, strlen((char *)anchor));
    hashtable_put(document->anchors, anchor, target);
}

static void add_to_mapping(Loader *context, Node *node)
{
    if(NULL == context->key_cache)
    {
        loader_trace("caching node (%p) as key for mapping context (%p)", node, context->current);
        context->key_cache = node;

        goto done;
    }

    if(!is_scalar(context->key_cache))
    {
        loader_debug("uh oh! found a non scalar mapping key");
        add_loader_error(context->errors, context->key_cache->position, ERR_NON_SCALAR_KEY);

        vector_append(context->detached, node);
        
        goto cleanup;
    }

    Scalar *scalar = scalar(context->key_cache);
    String *key = scalar_value(scalar);
    scalar->value = NULL;  // N.B. - ownership of value transferred to mapping

    Mapping *mapping = mapping(context->current);
    bool duplicate = mapping_contains(mapping, key);

    if(duplicate && DUPE_FAIL == context->strategy)
    {
        loader_debug("uh oh! a duplicate key found");
        add_loader_error(context->errors, scalar->position, ERR_DUPLICATE_KEY);

        vector_append(context->detached, node);
        
        goto cleanup;
    }
    else if(duplicate && DUPE_WARN == context->strategy)
    {
        const char *name = C(context->input_name);
        size_t line = scalar->position.line + 1;
        size_t offset  = scalar->position.offset + 1;
        fprintf(stderr, "%s:%zu:%zu: warning: duplicate mapping key \"%s\"\n", name, line, offset, C(key));
    }

    mapping_put(mapping, key, node);

  cleanup:
    dispose_node(context->key_cache);
    context->key_cache = NULL;
  done:
    node->parent = context->current;
}

static void add_node(Loader *context, Node *node, const yaml_event_t *event)
{
    switch(node_kind(context->current))
    {
        case DOCUMENT:
            loader_trace("adding node (%p) to document context (%p)", node, context->current);
            document_set_root(document(context->current), node);
            break;
        case SEQUENCE:
            loader_trace("adding node (%p) to sequence context (%p)", node, context->current);
            sequence_add(sequence(context->current), node);
            break;
        case MAPPING:
            loader_trace("adding node (%p) to mapping context (%p)", node, context->current);
            add_to_mapping(context, node);
            break;
        default:
            loader_debug("uh oh! an unsupported node kind has become the context node");
            add_loader_error(context->errors, position(event->start_mark), ERR_INTERNAL_CTX_NODE);
            vector_append(context->detached, node);
            break;
    }
}

static void start_document(Loader *context, const yaml_event_t *event)
{
    Document *document = make_document_node();
    document->position = position(event->start_mark);

    context->current = node(document);
    document_set_add(context->documents, document);

    loader_trace("started document (%p)", document);
}

static void end_document(Loader *context)
{
    loader_trace("completed document (%p)", context->current);

    context->current = NULL;
}

static void start_sequence(Loader *context, const yaml_event_t *event)
{
    Sequence *sequence = make_sequence_node();
    sequence->position = position(event->start_mark);

    if(NULL != event->data.sequence_start.tag)
    {
        size_t len = strlen((char *)event->data.sequence_start.tag);
        node_set_tag(sequence, event->data.sequence_start.tag, len);
    }
    set_anchor(context, node(sequence), event->data.sequence_start.anchor);

    add_node(context, node(sequence), event);
    context->current = node(sequence);

    loader_trace("started sequence (%p)", sequence);
}

static void end_sequence(Loader *context)
{
    Sequence *sequence = sequence(context->current);
    vector_trim(sequence->values);
    context->current = sequence->parent;

    loader_trace("completed sequence (%p) of length: %zu", sequence, node_size(sequence));
}

static void start_mapping(Loader *context, const yaml_event_t *event)
{
    Mapping *mapping = make_mapping_node();
    mapping->position = position(event->start_mark);

    if(NULL != event->data.mapping_start.tag)
    {
        size_t length = strlen((char *)event->data.mapping_start.tag);
        node_set_tag(mapping, event->data.mapping_start.tag, length);
    }
    set_anchor(context, node(mapping), event->data.mapping_start.anchor);

    add_node(context, node(mapping), event);
    context->current = node(mapping);

    loader_trace("started mapping (%p)", mapping);
}

static void end_mapping(Loader *context)
{
    Mapping *mapping = mapping(context->current);
    context->current = mapping->parent;

    loader_trace("completed mapping (%p) of length: %zu", mapping, node_size(mapping));
}

static void add_alias(Loader *context, const yaml_event_t *event)
{
    Document *document = current_document(context);
    Node *target = hashtable_get(document->anchors, event->data.alias.anchor);
    if(NULL == target)
    {
        loader_debug("uh oh! alias anchor is not known");
        add_loader_error(context->errors, position(event->start_mark), ERR_NO_ANCHOR_FOR_ALIAS);
        return;
    }

    for(Node *cur = node_parent(context->current); NULL != cur; cur = node_parent(cur))
    {
        if(cur == target)
        {
            loader_debug("uh oh! an alias loop was detected");
            add_loader_error(context->errors, position(event->start_mark), ERR_ALIAS_LOOP);
            return;
        }
    }

    Alias *alias = make_alias_node(target);
    alias->position = position(event->start_mark);

    add_node(context, node(alias), event);

    loader_trace("added alias \"%s\" for target (%p)", event->data.alias.anchor, target);
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

static ScalarKind tag_to_scalar_kind(const yaml_event_t *event)
{
    const yaml_char_t * tag = event->data.scalar.tag;
    if(0 == memcmp(YAML_NULL_TAG, tag, strlen(YAML_NULL_TAG)))
    {
        loader_trace("found yaml null tag for scalar \"%s\"", event->data.scalar.value);
        return SCALAR_NULL;
    }
    if(0 == memcmp(YAML_BOOL_TAG, tag, strlen(YAML_BOOL_TAG)))
    {
        loader_trace("found yaml boolean tag for scalar \"%s\"", event->data.scalar.value);
        return SCALAR_BOOLEAN;
    }
    if(0 == memcmp(YAML_STR_TAG, tag, strlen(YAML_STR_TAG)))
    {
        loader_trace("found yaml string tag for scalar \"%s\"", event->data.scalar.value);
        return SCALAR_STRING;
    }
    if(0 == memcmp(YAML_INT_TAG, tag, strlen(YAML_INT_TAG)))
    {
        loader_trace("found yaml integer tag for scalar \"%s\"", event->data.scalar.value);
        return SCALAR_INTEGER;
    }
    if(0 == memcmp(YAML_FLOAT_TAG, tag, strlen(YAML_FLOAT_TAG)))
    {
        loader_trace("found yaml float tag for scalar \"%s\"", event->data.scalar.value);
        return SCALAR_REAL;
    }
    if(0 == memcmp(YAML_TIMESTAMP_TAG, tag, strlen(YAML_TIMESTAMP_TAG)))
    {
        loader_trace("found yaml timestamp tag for scalar \"%s\"", event->data.scalar.value);
        return SCALAR_TIMESTAMP;
    }

    loader_trace("found non-yaml tag for scalar \"%s\", assuming string", event->data.scalar.value);
    return SCALAR_STRING;
}

static ScalarKind resolve_scalar_kind(const Loader *context, const yaml_event_t *event)
{
    ScalarKind kind = SCALAR_STRING;

    if(NULL != event->data.scalar.tag)
    {
        kind = tag_to_scalar_kind(event);
    }
    else if(YAML_SINGLE_QUOTED_SCALAR_STYLE == event->data.scalar.style ||
            YAML_DOUBLE_QUOTED_SCALAR_STYLE == event->data.scalar.style)
    {
        loader_trace("found quoted scalar string \"%s\"", event->data.scalar.value);
        kind = SCALAR_STRING;
    }
    else if(0 == memcmp("null", event->data.scalar.value, 4))
    {
        loader_trace("found scalar null");
        kind = SCALAR_NULL;
    }
    else if(0 == memcmp("true", event->data.scalar.value, 4) ||
            0 == memcmp("false", event->data.scalar.value, 5))
    {
        loader_trace("found scalar boolean \"%s\"", event->data.scalar.value);
        kind = SCALAR_BOOLEAN;
    }
    else if(match_integer((const char *)event->data.scalar.value))
    {
        loader_trace("found scalar integer \"%s\"", event->data.scalar.value);
        kind = SCALAR_INTEGER;
    }
    else if(match_decimal((const char *)event->data.scalar.value))
    {
        loader_trace("found scalar real \"%s\"", event->data.scalar.value);
        kind = SCALAR_REAL;
    }
    else if(match_timestamp((const char *)event->data.scalar.value))
    {
        loader_trace("found scalar timestamp \"%s\"", event->data.scalar.value);
        kind = SCALAR_TIMESTAMP;
    }
    else
    {
        loader_trace("found scalar string \"%s\"", event->data.scalar.value);
    }

    return kind;
}

static void add_scalar(Loader *context, const yaml_event_t *event)
{
    ScalarKind kind = resolve_scalar_kind(context, event);
    String *value = make_string_with_bytestring(event->data.scalar.value, event->data.scalar.length);
    Scalar *scalar = make_scalar_node(value, kind);
    scalar->position = position(event->start_mark);

    if(NULL != event->data.scalar.tag)
    {
        node_set_tag(scalar, event->data.scalar.tag, strlen((char *)event->data.scalar.tag));
    }
    set_anchor(context, node(scalar), event->data.scalar.anchor);

    add_node(context, node(scalar), event);

    loader_trace("added scalar (%p)", scalar);
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

    return true;
}

static void event_loop(Loader *context, yaml_parser_t *parser)
{
    yaml_event_t event;
    memset(&event, 0, sizeof(event));

    while(1)
    {
        if(!yaml_parser_parse(parser, &event))
        {
            interpret_yaml_error(context, parser);
            return;
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

    if(vector_is_empty(context.errors))
    {
        loader_debug("found %zu documents.", document_set_size(context.documents));

        if(0 == document_set_size(context.documents))
        {
            dispose_document_set(context.documents);
            add_loader_error(context.errors, NO_POSITION, ERR_NO_DOCUMENTS_FOUND);
            return fail(DocumentSet, context.errors);
        }

        dispose_vector(context.errors);
        return just(DocumentSet, context.documents);
    }

    vector_destroy(context.detached, node_destroyer);
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

    loader_debug("loading yaml from \"%s\"", C(input_name(input)));
    yaml_parser_set_input_string(&parser, (const unsigned char *)input->source.buffer, input_length(input));

    Maybe(DocumentSet) result = parse(input_name(input), &parser, strategy);

    yaml_parser_delete(&parser);

    return result;
}
