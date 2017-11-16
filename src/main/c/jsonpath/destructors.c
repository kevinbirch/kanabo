#include "jsonpath.h"

static inline void step_destructor(void *each);

static inline void jsonpath_free(JsonPath *path)
{
    if(NULL == path)
    {
        return;
    }
    vector_destroy(path->steps, step_destructor);
    free(path->steps);
    free(path);
}

static void predicate_free(Predicate *value)
{
    if(NULL == value)
    {
        return;
    }
    if(JOIN == predicate_kind(value))
    {
        jsonpath_free(value->join.left);
        jsonpath_free(value->join.right);
    }

    free(value);
}

static inline void step_free(Step *value)
{
    if(NULL == value)
    {
        return;
    }
    if(NAME_TEST == value->test.kind)
    {
        if(NULL != value->test.name.value)
        {
            free(value->test.name.value);
            value->test.name.value = NULL;
            value->test.name.length = 0;
        }
    }
    if(NULL != value->predicate)
    {
        predicate_free(value->predicate);
    }
    free(value);
}

static inline void step_destructor(void *each)
{
    step_free((Step *)each);
}

void dispose_path(JsonPath path)
{
    vector_destroy(path.steps, step_destructor);
    free(path.steps);
}
