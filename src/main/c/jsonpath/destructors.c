#include "jsonpath.h"

static void predicate_free(Predicate *self)
{
    if(NULL == self)
    {
        return;
    }

    if(JOIN == self->kind)
    {
        dispose_path(self->join.left);
        dispose_path(self->join.right);
    }

    free(self);
}

static inline void step_free(Step *self)
{
    if(NULL == self)
    {
        return;
    }

    if(NAME_TEST == self->test.kind)
    {
        if(NULL != self->test.name)
        {
            string_free(self->test.name);
            self->test.name = NULL;
        }
    }

    if(NULL != self->predicate)
    {
        predicate_free(self->predicate);
    }

    free(self);
}

static inline void step_destructor(void *each)
{
    step_free((Step *)each);
}

void dispose_path(JsonPath *path)
{
    if(NULL == path)
    {
        return;
    }

    vector_destroy(path->steps, step_destructor);
    free(path);    
}
