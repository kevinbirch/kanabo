#include "evaluator/predicate.h"

static bool apply_predicate(Node *value, void *argument, Nodelist *target);
static bool apply_wildcard_predicate(Node *value, Evaluator *evaluator, Nodelist *target);
static bool apply_subscript_predicate(const Sequence *value, Evaluator *evaluator, Nodelist *target);
static bool apply_slice_predicate(const Sequence *value, Evaluator *evaluator, Nodelist *target);
static bool apply_join_predicate(Node *value, Evaluator *evaluator, Nodelist *target);

bool evaluate_predicate(Evaluator *evaluator)
{
    evaluate_nodelist("predicate",
                      predicate_kind_name(current_step(evaluator)->predicate->kind),
                      apply_predicate);
}

bool apply_predicate(Node *each, void *argument, Nodelist *target)
{
    Evaluator *evaluator = (Evaluator *)argument;
    bool result = false;
    switch(current_step(evaluator)->predicate->kind)
    {
        case WILDCARD:
            evaluator_trace("evaluating wildcard predicate");
            result = apply_wildcard_predicate(each, evaluator, target);
            break;
        case SUBSCRIPT:
            evaluator_trace("evaluating subscript predicate");
            if(!is_sequence(each))
            {
                evaluator_trace("subscript predicate: node is not a sequence type, cannot use an index on it (kind: %d), dropping (%p)", node_kind(each), each);
                result = true;
            }
            else
            {
                result = apply_subscript_predicate(sequence(each), evaluator, target);
            }
            break;
        case SLICE:
            evaluator_trace("evaluating slice predicate");
            if(!is_sequence(each))
            {
                evaluator_trace("slice predicate: node is not a sequence type, cannot use a slice on it (kind: %d), dropping (%p)", node_kind(each), each);
                result = true;
            }
            else
            {
                result = apply_slice_predicate(sequence(each), evaluator, target);
            }
            break;
        case JOIN:
            evaluator_trace("evaluating join predicate");
            result = apply_join_predicate(each, evaluator, target);
            break;
    }
    return result;
}

static bool apply_wildcard_predicate(Node *value, Evaluator *evaluator, Nodelist *target)
{
    bool result = false;
    switch(node_kind(value))
    {
        case SCALAR:
            trace_string("wildcard predicate: adding scalar '%s' (%p)", scalar_value(scalar((Node *)value)), node_size(value), value);
            nodelist_add(target, value);
            return true;
            break;
        case MAPPING:
            evaluator_trace("wildcard predicate: adding mapping (%p)", value);
            nodelist_add(target, value);
            return true;
            break;
        case SEQUENCE:
            evaluator_trace("wildcard predicate: adding %zu sequence (%p) items", node_size(value), value);
            result = sequence_iterate(sequence((Node *)value), add_to_nodelist_sequence_iterator, target);
            break;
        case DOCUMENT:
            evaluator_error("wildcard predicate: uh-oh! found a document node (%p), aborting...", value);
            evaluator->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
        case ALIAS:
            evaluator_trace("wildcard predicate: resolving alias (%p)", value);
            result = apply_wildcard_predicate(alias_target(alias((Node *)value)), evaluator, target);
            break;
    }
    return result;
}

static bool apply_subscript_predicate(const Sequence *value, Evaluator *evaluator, Nodelist *target)
{
    Predicate *subscript = current_step(evaluator)->predicate;
    int64_t index = subscript_predicate_index(subscript);
    uint64_t abs = (uint64_t)index;
    if(abs > node_size(value))
    {
        evaluator_trace("subscript predicate: index %zu not valid for sequence (length: %zd), dropping (%p)", index, node_size(value), value);
        return true;
    }
    Node *selected = sequence_get(value, index);
    evaluator_trace("subscript predicate: adding index %zu (%p) from sequence (%p) of %zd items", index, selected, value, node_size(value));
    nodelist_add(target, selected);
    return true;
}

static bool apply_slice_predicate(const Sequence *value, Evaluator *evaluator, Nodelist *target)
{
    Predicate *slice = current_step(evaluator)->predicate;
    int64_t from = 0, to = 0, increment = 0;
    normalize_interval(value, slice, &from, &to, &increment);
    evaluator_trace("slice predicate: using normalized interval [%d:%d:%d]", from, to, increment);

    for(int64_t i = from; 0 > increment ? i >= to : i < to; i += increment)
    {
        Node *selected = sequence_get(value, i);
        if(NULL == selected)
        {
            break;
        }
        evaluator_trace("slice predicate: adding index: %d (%p)", i, selected);
        nodelist_add(target, selected);
    }
    return true;
}

static bool apply_join_predicate(Node *value, Evaluator *evaluator, Nodelist *target)
{
    evaluator_trace("join predicate: evaluating axes (_, _)");

    // xxx - implement me!
    evaluator_error("join predicate: uh-oh! not implemented yet, aborting...");
    evaluator->code = ERR_UNSUPPORTED_PATH;
    return false;
}

static int normalize_extent(bool specified_p, int64_t given, int64_t fallback, int64_t limit)
{
    if(!specified_p)
    {
        evaluator_trace("slice predicate: (normalizer) no value specified, defaulting to %zd", fallback);
        return fallback;
    }
    int64_t result = 0 > given ? given + limit: given;
    if(0 > result)
    {
        evaluator_trace("slice predicate: (normalizer) negative value, clamping to zero");
        return 0;
    }
    if(limit < result)
    {
        evaluator_trace("slice predicate: (normalizer) value over limit, clamping to %zd", limit);
        return limit;
    }
    evaluator_trace("slice predicate: (normalizer) constrained to %zd", result);
    return result;
}

static int64_t normalize_from(const predicate *slice, const Sequence *value)
{
    evaluator_trace("slice predicate: normalizing from, specified: %s, value: %d", slice_predicate_has_from(slice) ? "yes" : "no", slice_predicate_from(slice));
    int64_t length = (int64_t)node_size(value);
    return normalize_extent(slice_predicate_has_from(slice), slice_predicate_from(slice), 0, length);
}

static int64_t normalize_to(const predicate *slice, const Sequence *value)
{
    evaluator_trace("slice predicate: normalizing to, specified: %s, value: %d", slice_predicate_has_to(slice) ? "yes" : "no", slice_predicate_to(slice));
    int64_t length = (int64_t)node_size(value);
    return normalize_extent(slice_predicate_has_to(slice), slice_predicate_to(slice), length, length);
}

#ifdef USE_LOGGING
static inline void trace_interval(const Sequence *value, predicate *slice)
{
    static const char * fmt = "slice predicate: evaluating interval [%s:%s:%s] on sequence (%p) of %zd items";
    static const char * extent_fmt = "%" PRIdFAST32;
    size_t len = (unsigned)lrint(floor(log10((float)ULLONG_MAX))) + 1;
    char from_repr[len + 1];
    if(slice_predicate_has_from(slice))
    {
        snprintf(from_repr, len, extent_fmt, slice_predicate_from(slice));
    }
    else
    {
        from_repr[0] = '_';
        from_repr[1] = '\0';
    }
    char to_repr[len + 1];
    if(slice_predicate_has_to(slice))
    {
        snprintf(to_repr, len, extent_fmt, slice_predicate_to(slice));
    }
    else
    {
        to_repr[0] = '_';
        to_repr[1] = '\0';
    }
    char step_repr[len + 1];
    if(slice_predicate_has_step(slice))
    {
        snprintf(step_repr, len, extent_fmt, slice_predicate_step(slice));
    }
    else
    {
        step_repr[0] = '_';
        step_repr[1] = '\0';
    }
    evaluator_trace(fmt, from_repr, to_repr, step_repr, value, node_size(value));
}
#else
#define trace_interval(...)
#endif

static void normalize_interval(const Sequence *value, predicate *slice, int64_t *from_val, int64_t *to_val, int64_t *step_val)
{
    trace_interval(value, slice);
    *step_val = slice_predicate_has_step(slice) ? slice_predicate_step(slice) : 1;
    *from_val = 0 > *step_val ? normalize_to(slice, value) - 1 : normalize_from(slice, value);
    *to_val   = 0 > *step_val ? normalize_from(slice, value) : normalize_to(slice, value);
}
