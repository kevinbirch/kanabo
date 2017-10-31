#include "document.h"
#include "conditions.h"

node *document_root(const node *document)
{
    PRECOND_NONNULL_ELSE_NULL(document);
    PRECOND_ELSE_NULL(DOCUMENT == node_kind(document));

    return document->content.target;
}

bool document_set_root(node *document, node *root)
{
    PRECOND_NONNULL_ELSE_FALSE(document, root);
    PRECOND_ELSE_FALSE(DOCUMENT == node_kind(document));

    document->content.target = root;
    root->parent = document;
    return true;
}
