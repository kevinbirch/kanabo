#include "jsonpath.h"
#include "conditions.h"

static const char * const PATH_KIND_NAMES[] =
{
    "absolute path",
    "relative path"
};

struct context_adapter
{
    void *original_context;
    path_iterator iterator;
};

static bool iterator_adapter(void *each, void *context);


enum path_kind path_kind(const JsonPath *path)
{
    return path->kind;
}

size_t path_length(const JsonPath *path)
{
    PRECOND_NONNULL_ELSE_ZERO(path);
    return path->length;
}

Step *path_get(const JsonPath *path, size_t index)
{
    PRECOND_NONNULL_ELSE_NULL(path);
    PRECOND_NONNULL_ELSE_NULL(path->steps);
    PRECOND_ELSE_NULL(0 != path->length, index < path->length);
    return vector_get(path->steps, index);
}

static bool iterator_adapter(void *each, void *context)
{
    struct context_adapter *adapter = (struct context_adapter *)context;
    return adapter->iterator((Step *)each, adapter->original_context);
}

bool inline path_iterate(const JsonPath *path, path_iterator iterator, void *context)
{
    PRECOND_NONNULL_ELSE_FALSE(path, iterator);

    return vector_iterate(path->steps, iterator_adapter, &(struct context_adapter){context, iterator});
}

const char *path_kind_name(enum path_kind value)
{
    return PATH_KIND_NAMES[value];
}
