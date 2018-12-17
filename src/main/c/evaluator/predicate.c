#include <inttypes.h>

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
            trace_string("wildcard predicate: adding scalar '%s' (%p)", scalar_value(scalar(value)), node_size(value), value);
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
            result = sequence_iterate(sequence(value), add_to_nodelist_sequence_iterator, target);
            break;
        case DOCUMENT:
            evaluator_error("wildcard predicate: uh-oh! found a document node (%p), aborting...", value);
            evaluator->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
        case ALIAS:
            evaluator_trace("wildcard predicate: resolving alias (%p)", value);
            result = apply_wildcard_predicate(alias_target(alias(value)), evaluator, target);
            break;
    }

    return result;
}

static bool apply_subscript_predicate(const Sequence *value, Evaluator *evaluator, Nodelist *target)
{
    Predicate *subscript = current_step(evaluator)->predicate;
    int64_t index = subscript_predicate_index(subscript);
    // xxx - must support negative indices here
    uint64_t abs = (uint64_t)index;
    if(abs > node_size(value))
    {
        evaluator_trace("subscript predicate: index %zu not valid for sequence (length: %zd), dropping (%p)", index, node_size(value), value);
        return true;
    }
    Node *selected = sequence_get(value, abs);
    evaluator_trace("subscript predicate: adding index %zu (%p) from sequence (%p) of %zd items", index, selected, value, node_size(value));
    nodelist_add(target, selected);
    return true;
}

typedef int64_t (*get_extent)(const Predicate *);

static inline uint64_t normalize_extent(const Sequence *seq, const Predicate *pred, get_extent get)
{
    int64_t length = (int64_t)node_size(seq);
    int64_t given = get(pred);
    if(0 > given)
    {
        if(given > (0 - length))
        {
            return 0;
        }

        return (uint64_t)(length + given);
    }

    return (uint64_t)given;

}

static inline uint64_t normalize_from(const Sequence *seq, const Predicate *slice)
{
    if(!slice_predicate_has_from(slice))
    {
        return 0;
    }

    uint64_t from = normalize_extent(seq, slice, slice_predicate_from);

    uint64_t length = node_size(seq);
    if(from >= length)
    {
        return length - 1;
    }

    return from;
}

static inline uint64_t normalize_to(const Sequence *seq, const Predicate *slice)
{
    uint64_t length = node_size(seq);
    if(!slice_predicate_has_to(slice))
    {
        return length;
    }

    uint64_t to = normalize_extent(seq, slice, slice_predicate_to);

    if(to > length)
    {
        return length;
    }

    return to;
}

static bool apply_slice_predicate(const Sequence *seq, Evaluator *evaluator, Nodelist *target)
{
    uint64_t from, to;

    Predicate *slice = current_step(evaluator)->predicate;
    int64_t step = slice_predicate_has_step(slice) ? slice_predicate_step(slice) : 1;
    if(0 == step)
    {
        evaluator_error("slice predicate: zero step, aborting...");
        return false;
    }
    
    from = normalize_from(seq, slice);
    to = normalize_to(seq, slice);

    evaluator_trace("slice predicate: normalized interval [%d:%d:%d]", from, to, step);


    if(step > 0)
    {
        if(to > from)
        {
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
            evaluator_trace("slice predicate: adding index: %d (%p)", i, selected);
            nodelist_add(target, selected);
        }

        return true;
    }

    if(from > to)
    {
        return false;
    }

    for(size_t i = from; i < to; i += (uint64_t)step)
    {
        Node *selected = sequence_get(seq, i);
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
