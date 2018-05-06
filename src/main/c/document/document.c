#include "document.h"
#include "conditions.h"

static bool document_equals(const Node *one, const Node *two)
{
    return node_equals(document_root((const Document *)one),
                       document_root((const Document *)two));
}

static void document_free(Node *value)
{
    Document *doc = (Document *)value;
    node_free(doc->root);
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
    Document *self = calloc(1, sizeof(Document));
    if(NULL != self)
    {
        node_init(self, DOCUMENT);
        self->base.vtable = &document_vtable;
    }

    return self;
}

Node *document_root(const Document *self)
{
    PRECOND_NONNULL_ELSE_NULL(self);

    return self->root;
}

bool document_set_root(Document *self, Node *root)
{
    PRECOND_NONNULL_ELSE_FALSE(self, root);

    self->root = root;
    root->parent = node(self);
    return true;
}
