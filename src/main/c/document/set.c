#include "conditions.h"
#include "document.h"
#include "panic.h"
#include "xalloc.h"

DocumentSet *make_document_set(void)
{
    DocumentSet *self = xcalloc(sizeof(DocumentSet));

    self->values = make_vector_with_capacity(1);
    if(NULL == self->values)
    {
        panic("document: document set: allocate document set vector failed");
    }

    return self;
}

static void freedom_iterator(void *each)
{
    dispose_node(each);
}

void dispose_document_set(DocumentSet *self)
{
    if(NULL == self)
    {
        return;
    }

    dispose_string(self->input_name);
    vector_destroy(self->values, freedom_iterator);
    free(self);
}

Node *document_set_get_root(const DocumentSet *self, size_t index)
{
    if(NULL == self)
    {
        return NULL;
    }

    Node *result = NULL;

    Document *doc = document_set_get(self, index);
    if(NULL != doc)
    {
        result = document_root(doc);
    }

    return result;
}

bool document_set_add(DocumentSet *self, Document *doc)
{
    ENSURE_NONNULL_ELSE_FALSE(self, doc);

    return vector_add(self->values, doc);
}
