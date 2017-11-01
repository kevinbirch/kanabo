#include "document.h"

static inline void sequence_free(node *sequence);
static inline void mapping_free(node *mapping);

static bool sequence_freedom_iterator(void *each, void *context);
static bool mapping_freedom_iterator(void *key, void *value, void *context);

void model_free(document_model *model)
{
    if(NULL == model)
    {
        return;
    }
    vector_iterate(model->documents, sequence_freedom_iterator, NULL);
    vector_free(model->documents);
    model->documents = NULL;
    
    free(model);
}

void node_free(node *value)
{
    if(NULL == value)
    {
        return;
    }
    switch(node_kind(value))
    {
        case DOCUMENT:
            node_free(value->content.target);
            value->content.target = NULL;
            break;
        case SCALAR:
            free(value->content.scalar.value);
            value->content.scalar.value = NULL;
            break;
        case SEQUENCE:
            sequence_free(value);
            break;
        case MAPPING:
            mapping_free(value);
            break;
        case ALIAS:
            break;
    }
    free(value->tag.name);
    free(value->anchor);
    free(value);
}

static bool sequence_freedom_iterator(void *each, void *context)
{
    node_free((node *)each);

    return true;
}
static inline void sequence_free(node *sequence)
{
    if(NULL == sequence->content.sequence)
    {
        return;
    }
    vector_iterate(sequence->content.sequence, sequence_freedom_iterator, NULL);
    vector_free(sequence->content.sequence);
    sequence->content.sequence = NULL;
}

static bool mapping_freedom_iterator(void *key, void *value, void *context)
{
    node_free((node *)key);
    node_free((node *)value);
    
    return true;
}

static inline void mapping_free(node *mapping)
{
    if(NULL == mapping->content.mapping)
    {
        return;
    }
    
    hashtable_iterate(mapping->content.mapping, mapping_freedom_iterator, NULL);
    hashtable_free(mapping->content.mapping);
    mapping->content.mapping = NULL;
}
