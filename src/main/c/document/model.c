#include "document.h"
#include "conditions.h"

node *model_document(const document_model *model, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(model);
    PRECOND_ELSE_NULL(index < model_document_count(model));

    return vector_get(model->documents, index);
}

node *model_document_root(const document_model *model, size_t index)
{
    node *document = model_document(model, index);
    node *result = NULL;
    
    if(NULL != document)
    {
        result = document_root(document);
    }

    return result;
}

size_t model_document_count(const document_model *model)
{
    PRECOND_NONNULL_ELSE_ZERO(model);

    return vector_length(model->documents);
}

bool model_add(document_model *model, node *document)
{
    PRECOND_NONNULL_ELSE_FALSE(model, document);
    PRECOND_ELSE_FALSE(DOCUMENT == node_kind(document));

    return vector_add(model->documents, document);
}

