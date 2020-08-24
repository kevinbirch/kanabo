#include "conditions.h"
#include "document.h"
#include "xalloc.h"

DocumentSet *make_document_set(void)
{
    DocumentSet *self = xcalloc(sizeof(DocumentSet));
    self->values = make_vector_with_capacity(1);

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

static bool docset_dumper(void *each, void *context)
{
    fprintf(stdout, "---\n");
    node_dump(each, true);

    return true;
}

void document_set_dump(const DocumentSet *model)
{
    vector_iterate(model->values, docset_dumper, NULL);
}
