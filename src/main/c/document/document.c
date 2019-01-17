#include "conditions.h"
#include "document.h"
#include "xalloc.h"

static bool document_equals(const Node *one, const Node *two)
{
    return node_equals(document_root((const Document *)one),
                       document_root((const Document *)two));
}

static void document_free(Node *value)
{
    Document *doc = (Document *)value;
    dispose_node(doc->root);
    doc->root = NULL;
}

static size_t document_size(const Node *self)
{
    return NULL == ((Document *)self)->root ? 0 : 1;
}

static const struct vtable_s document_vtable = 
{
    document_free,
    document_size,
    document_equals
};

Document *make_document_node(void)
{
    Document *self = xcalloc(sizeof(Document));
    node_init(node(self), DOCUMENT, &document_vtable);

    return self;
}

void document_set_root(Document *self, Node *root)
{
    ENSURE_NONNULL_ELSE_VOID(self, root);

    self->root = root;
    root->parent = node(self);
}
