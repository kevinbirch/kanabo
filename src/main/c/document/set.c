#include "conditions.h"
#include "document.h"

static bool freedom_iterator(void *each, void *context __attribute__((unused)))
{
    dispose_node(each);

    return true;
}

void dispose_document_set(DocumentSet *self)
{
    if(NULL == self)
    {
        return;
    }
    vector_iterate(self, freedom_iterator, NULL);
    dispose_vector(self);
}

Node *document_set_get_root(const DocumentSet *self, size_t index)
{
    Document *doc = document_set_get(self, index);
    Node *result = NULL;

    if(NULL != doc)
    {
        result = document_root(doc);
    }

    return result;
}

bool document_set_add(DocumentSet *self, Document *doc)
{
    ENSURE_NONNULL_ELSE_FALSE(self, doc);

    return vector_add(self, doc);
}
