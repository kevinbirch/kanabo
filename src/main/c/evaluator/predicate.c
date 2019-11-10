#include <inttypes.h>

#include "evaluator/predicate.h"

static bool apply_wildcard_predicate(Node *value, Evaluator *evaluator, Nodelist *target)
{
    bool result = true;

    switch(node_kind(value))
    {
        case SCALAR:
            evaluator_trace("wildcard predicate: adding scalar \"%s\" (%p)", C(scalar_value(value)), (void *)value);
            nodelist_add(target, value);
            break;
        case MAPPING:
            evaluator_trace("wildcard predicate: adding mapping (%p)", (void *)value);
            nodelist_add(target, value);
            break;
        case SEQUENCE:
            evaluator_trace("wildcard predicate: adding %zu sequence (%p) items", node_size(value), (void *)value);
            result = sequence_iterate(sequence(value), add_to_nodelist_sequence_iterator, target);
            break;
        case ALIAS:
            evaluator_trace("wildcard predicate: resolving alias (%p)", (void *)value);
            result = apply_wildcard_predicate(alias_target(value), evaluator, target);
            break;
        case DOCUMENT:
            evaluator_debug("wildcard predicate: uh-oh! found a document node (%p)", (void *)value);
            evaluator->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            result = false;
            break;
    }

    return result;
}

static bool apply_subscript_predicate(const Sequence *seq, Evaluator *evaluator, Nodelist *target)
{
    Predicate *subscript = current_step(evaluator)->predicate;
    int64_t index = subscript_predicate_index(subscript);
    size_t length = node_size(seq);

    uint64_t normal = (uint64_t)imaxabs(index);

    if(normal > length)
    {
        evaluator_debug("subscript predicate: index %"PRId64" out of range for sequence (length: %zu)", index, length);
        evaluator->code = ERR_SUBSCRIPT_PREDICATE;
        return false;
    }

    if(0 > index)
    {
        normal = length - normal;
    }

    Node *selected = sequence_get(seq, normal);
    evaluator_trace("subscript predicate: adding index %"PRId64" (%p) from sequence (%p) of %zu items", normal, (void *)selected, (void *)seq, length);
    nodelist_add(target, selected);

    return true;
}

static inline uint64_t normalize_extent(int64_t extent, int64_t length)
{
    if(0 < extent)
    {
        return (uint64_t)extent;
    }

    if(extent < (0 - length))
    {
        return 0;
    }

    return (uint64_t)(length + extent);
}

static inline uint64_t normalize_from(const Predicate *slice, uint64_t length)
{
    if(!slice_predicate_has_from(slice))
    {
        if(0 > slice_predicate_step(slice))
        {
            return length - 1;
        }
        return 0;
    }

    int64_t from = slice_predicate_from(slice);
    uint64_t normal = normalize_extent(from, (int64_t)length);

    if(normal >= length)
    {
        if(0 > from)
        {
            return 0;
        }
        return length - 1;
    }

    return normal;
}

static inline uint64_t normalize_to(const Predicate *slice, uint64_t length)
{
    if(!slice_predicate_has_to(slice))
    {
        if(0 > slice_predicate_step(slice))
        {
            return 0;
        }
        return length;
    }

    int64_t to = slice_predicate_to(slice);
    uint64_t normal = normalize_extent(to, (int64_t)length);

    if(normal > length)
    {
        if(0 > to)
        {
            return 0;
        }
        return length;
    }

    return normal;
}

static bool apply_slice_predicate(const Sequence *seq, Evaluator *evaluator, Nodelist *target)
{
    Predicate *slice = current_step(evaluator)->predicate;
    int64_t step = slice_predicate_step(slice);
    if(0 == step)
    {
        evaluator_debug("slice predicate: step is zero");
        evaluator->code = ERR_SLICE_PREDICATE_ZERO_STEP;
        return false;
    }

    uint64_t length = node_size(seq);

    uint64_t from = normalize_from(slice, length);
    uint64_t to = normalize_to(slice, length);

    evaluator_trace("slice predicate: normalized interval [%"PRIu64":%"PRIu64":%"PRIu64"]", from, to, step);

    if(step < 0)
    {
        if(from < to)
        {
            evaluator_debug("step < 0 && from < to, aboring...");
            evaluator->code = ERR_SLICE_PREDICATE_DIRECTION;
            return false;
        }

        uint64_t stride = (uint64_t)imaxabs(step);
        for(size_t i = from; i >= to; i -= stride)
        {
            Node *selected = sequence_get(seq, i);
            if(NULL == selected)
            {
                break;
            }
            evaluator_trace("slice predicate: adding index: %zu (%p)", i, (void *)selected);
            nodelist_add(target, selected);
        }

        return true;
    }

    if(from > to)
    {
        evaluator_debug("step > 0 && from > to, aboring...");
        evaluator->code = ERR_SLICE_PREDICATE_DIRECTION;
        return false;
    }

    for(size_t i = from; i < to; i += (uint64_t)step)
    {
        Node *selected = sequence_get(seq, i);
        if(NULL == selected)
        {
            break;
        }
        evaluator_trace("slice predicate: adding index: %zu (%p)", i, (void *)selected);
        nodelist_add(target, selected);
    }

    return true;
}

static bool apply_join_predicate(Node *value, Evaluator *evaluator, Nodelist *target)
{
    evaluator_trace("join predicate: evaluating axes (_, _)");

    // xxx - implement me!
    evaluator->code = ERR_UNSUPPORTED_PATH;

    return false;
}

static bool apply_predicate(Node *each, void *argument, Nodelist *target)
{
    Evaluator *evaluator = (Evaluator *)argument;
    bool result = true;

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
                evaluator_trace("subscript predicate: node is not a sequence type, cannot use an index on it (kind: %d), dropping (%p)", node_kind(each), (void *)each);
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
                evaluator_trace("slice predicate: node is not a sequence type, cannot use a slice on it (kind: %d), dropping (%p)", node_kind(each), (void *)each);
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

bool evaluate_predicate(Evaluator *evaluator)
{
    evaluate_nodelist("predicate",
                      predicate_kind_name(current_step(evaluator)->predicate->kind),
                      apply_predicate);
}
