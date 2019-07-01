#include "conditions.h"
#include "document.h"
#include "xalloc.h"

static bool document_equals(const Node *one, const Node *two)
{
    return node_equals(document_root((const Document *)one),
                       document_root((const Document *)two));
}

static void tag_destroyer(void *each)
{
    TagDirective *td = (TagDirective *)each;
    dispose_string(td->handle);
    dispose_string(td->prefix);
    free(td);
}

static void document_free(Node *value)
{
    Document *doc = (Document *)value;
    dispose_node(doc->root);
    doc->root = NULL;

    dispose_hashtable(doc->yaml.anchors);  // N.B. - keys and values are owned by nodes
    doc->yaml.anchors = NULL;

    vector_destroy(doc->yaml.tags, tag_destroyer);
    doc->yaml.tags = NULL;
}

static size_t document_size(const Node *self)
{
    return NULL == ((Document *)self)->root ? 0 : 1;
}

static String *document_repr(const Node *value)
{
    Document *self = (Document *)value;

    const char *name = "NULL";
    if(NULL != self->root)
    {
        name = node_kind_name(self->root);
    }

    return format("<Document root: %s, pos: %zd:%zd>", name, self->position.line, self->position.offset);
}

static void document_dump(const Node *value, bool pad)
{
    String *repr = document_repr(value);
    fprintf(stdout, "%s\n", C(repr));
    dispose_string(repr);

    node_dump(document(value)->root, true);
}

static const struct vtable_s document_vtable = 
{
    document_free,
    document_size,
    document_equals,
    document_repr,
    document_dump
};

Document *make_document_node(void)
{
    Document *self = xcalloc(sizeof(Document));
    node_init(node(self), DOCUMENT, &document_vtable);
    self->depth = 0;

    return self;
}

void document_set_root(Document *self, Node *root)
{
    ENSURE_NONNULL_ELSE_VOID(self, root);

    self->root = root;
    root->document = self;
    root->parent = node(self);
    root->depth = 0;
}

static bool anchor_comparitor(const void *key1, const void *key2)
{
    return string_equals((String *)key1, (String *)key2);
}

static hashcode anchor_hash(const void *key)
{
    String *value = (String *)key;
    return fnv1a_string_buffer_hash(strdta(value), strlen(value));    
}

void document_track_anchor(Document *self, uint8_t *value, Node *target)
{
    ENSURE_NONNULL_ELSE_VOID(self, value, target);

    if(NULL == self->yaml.anchors)
    {
        self->yaml.anchors = make_hashtable_with_function(anchor_comparitor, anchor_hash);
    }

    String *anchor = make_string_with_bytestring(value, strlen((char *)value));
    node_set_anchor(target, anchor);
    hashtable_put(self->yaml.anchors, anchor, target);
}

Node *document_resolve_anchor(Document *self, uint8_t *value)
{
    ENSURE_NONNULL_ELSE_NULL(self, value);

    String *anchor = make_string_with_bytestring(value, strlen((char *)value));
    Node *target = hashtable_get(self->yaml.anchors, anchor);
    dispose_string(anchor);

    return target;
}
