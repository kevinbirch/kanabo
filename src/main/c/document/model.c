#include "document.h"
#include "conditions.h"

static bool freedom_iterator(void *each, void *context __attribute__((unused)))
{
    node_free(each);

    return true;
}

void model_free(DocumentModel *self)
{
    if(NULL == self)
    {
        return;
    }
    vector_iterate(self, freedom_iterator, NULL);
    vector_free(self);
}

Node *model_document_root(const DocumentModel *self, size_t index)
{
    Document *doc = model_document(self, index);
    Node *result = NULL;

    if(NULL != doc)
    {
        result = document_root(doc);
    }

    return result;
}

bool model_add(DocumentModel *self, Document *doc)
{
    PRECOND_NONNULL_ELSE_FALSE(self, doc);

    return vector_add(self, doc);
}
