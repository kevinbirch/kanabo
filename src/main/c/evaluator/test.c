#include "evaluator/test.h"

struct meta_context
{
    Evaluator *evaluator;
    Nodelist  *target;
};

typedef struct meta_context meta_context;

static bool recursive_test_sequence_iterator(Node *each, void *context)
{
    meta_context *iterator_context = (meta_context *)context;
    return apply_recursive_node_test(each, iterator_context->evaluator, iterator_context->target);
}

static bool recursive_test_map_iterator(Scalar *key, Node *value, void *context)
{
    meta_context *iterator_context = (meta_context *)context;
    return apply_recursive_node_test(value, iterator_context->evaluator, iterator_context->target);
}

static bool add_values_to_nodelist_map_iterator(Scalar *key, Node *value, void *context)
{
    meta_context *iterator_context = (meta_context *)context;
    bool result = true;

    switch(node_kind(value))
    {
        case SCALAR:
        case MAPPING:
        case SEQUENCE:
            evaluator_tracef("wildcard test: adding mapping value: %s (%p)", node_kind_name(value), (void *)value);
            nodelist_add(iterator_context->target, value);
            break;
        case ALIAS:
            evaluator_tracef("wildcard test: resolving alias (%p)", (void *)value);
            result = add_values_to_nodelist_map_iterator(key, alias_target(alias(value)), context);
            break;
        case DOCUMENT:
            evaluator_debugf("wildcard test: uh-oh! found a document node (%p)", (void *)value);
            iterator_context->evaluator->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            result = false;
            break;
    }

    return result;
}

static bool apply_greedy_wildcard_test(Node *each, void *argument, Nodelist *target)
{
    Evaluator *evaluator = (Evaluator *)argument;
    bool result = true;

    switch(node_kind(each))
    {
        case MAPPING:
            evaluator_tracef("wildcard test: adding %zu mapping values (%p)", node_size(each), (void *)each);
            result = mapping_iterate(mapping(each), add_values_to_nodelist_map_iterator, &(meta_context){evaluator, target});
            break;
        case SEQUENCE:
            evaluator_tracef("wildcard test: adding %zu sequence items (%p)", node_size(each), (void *)each);
            result = sequence_iterate(sequence(each), add_to_nodelist_sequence_iterator, target);
            break;
        case SCALAR:
            evaluator_tracef("wildcard test: adding scalar: \"%s\" (%p)", C(scalar_value(scalar(each))), (void *)each);
            nodelist_add(target, each);
            break;
        case ALIAS:
            evaluator_tracef("wildcard test: resolving alias (%p)", (void *)each);
            result = apply_greedy_wildcard_test(alias_target(alias(each)), argument, target);
            break;
        case DOCUMENT:
            evaluator_debugf("wildcard test: uh-oh! found a document node somehow (%p)", (void *)each);
            evaluator->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            result = false;
            break;
    }

    return result;
}

static bool apply_recursive_wildcard_test(Node *each, void *argument, Nodelist *target)
{
    Evaluator *evaluator = (Evaluator *)argument;
    bool result = true;

    switch(node_kind(each))
    {
        case MAPPING:
            evaluator_tracef("recurisve wildcard test: adding mapping node (%p)", (void *)each);
            nodelist_add(target, each);
            break;
        case SEQUENCE:
            evaluator_tracef("recurisve wildcard test: adding sequence node (%p)", (void *)each);
            nodelist_add(target, each);
            break;
        case SCALAR:
            evaluator_tracef("recurisve wildcard test: adding scalar: \"%s\" (%p)", C(scalar_value(scalar(each))), (void *)each);
            nodelist_add(target, each);
            break;
        case ALIAS:
            evaluator_tracef("recurisve wildcard test: resolving alias (%p)", (void *)each);
            result = apply_recursive_wildcard_test(alias_target(alias(each)), argument, target);
            break;
        case DOCUMENT:
            evaluator_debugf("recurisve wildcard test: uh oh! found a document node somehow (%p)", (void *)each);
            evaluator->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            result = false;
            break;
    }

    return result;
}

static bool apply_type_test(Node *each, void *argument, Nodelist *target)
{
    bool match = false;
    if(is_alias(each))
    {
        evaluator_tracef("type test: resolved alias from: (%p) to: (%p)",
                        (void *)each, (void *)alias_target((alias(each))));
        return apply_type_test(alias_target(alias(each)), argument, target);
    }

    Evaluator *evaluator = (Evaluator *)argument;
    switch(current_step(evaluator)->test.type)
    {
        case OBJECT_TEST:
            evaluator_trace("type test: testing for an object");
            match = is_mapping(each);
            break;
        case ARRAY_TEST:
            evaluator_trace("type test: testing for an array");
            match = is_sequence(each);
            break;
        case STRING_TEST:
            evaluator_trace("type test: testing for a string");
            match = is_string(each);
            break;
        case NUMBER_TEST:
            evaluator_trace("type test: testing for a number");
            match = is_number(each);
            break;
        case BOOLEAN_TEST:
            evaluator_trace("type test: testing for a boolean");
            match = is_boolean(each);
            break;
        case NULL_TEST:
            evaluator_trace("type test: testing for a null");
            match = is_null(each);
            break;
    }

    if(match)
    {
        evaluator_tracef("type test: match! adding node (%p)", (void *)each);
        nodelist_add(target, each);
    }
    else
    {
        const char *name = is_scalar(each) ? scalar_kind_name(scalar(each)) : node_kind_name(each);
        evaluator_tracef("type test: no match (actual: %s). dropping (%p)", name, (void *)each);
    }

    return true;
}

static bool apply_name_test(Node *each, void *argument, Nodelist *target)
{
    Evaluator *evaluator = (Evaluator *)argument;
    Step *context_step = current_step(evaluator);

    evaluator_tracef("name test: using key \"%s\"", C(name_test_step_name(context_step)));

    if(!is_mapping(each))
    {
        evaluator_tracef("name test: node is not a mapping type, cannot use a key on it (kind: %d), dropping (%p)", node_kind(each), (void *)each);
        return true;
    }

    Mapping *map = mapping(each);
    Node *value = mapping_lookup(map, name_test_step_name(context_step));

    if(NULL == value)
    {
        evaluator_tracef("name test: key not found in mapping, dropping (%p)", (void *)each);
        return true;
    }

    evaluator_tracef("name test: match! adding node (%p)", (void *)value);

    if(is_alias(value))
    {
        evaluator_tracef("name test: resolved alias from: (%p) to: (%p)",
                        (void *)value, (void *)alias_target(alias(value)));
        value = alias_target(alias(value));
    }

    nodelist_add(target, value);

    return true;
}

bool apply_recursive_node_test(Node *each, void *argument, Nodelist *target)
{
    Evaluator *evaluator = (Evaluator *)argument;
    bool result = true;

    if(!is_alias(each))
    {
        result = apply_node_test(each, argument, target);
    }

    if(result)
    {
        switch(node_kind(each))
        {
            case MAPPING:
                evaluator_tracef("recursive step: processing %zu mapping values (%p)", node_size(each), (void *)each);
                result = mapping_iterate(mapping(each), recursive_test_map_iterator, &(meta_context){evaluator, target});
                break;
            case SEQUENCE:
                evaluator_tracef("recursive step: processing %zu sequence items (%p)", node_size(each), (void *)each);
                result = sequence_iterate(sequence(each), recursive_test_sequence_iterator, &(meta_context){evaluator, target});
                break;
            case SCALAR:
                evaluator_tracef("recursive step: found scalar, recursion finished on this path (%p)", (void *)each);
                break;
            case ALIAS:
                evaluator_tracef("recursive step: resolving alias (%p)", (void *)each);
                result = apply_recursive_node_test(alias_target(alias(each)), argument, target);
                break;
            case DOCUMENT:
                evaluator_debugf("recursive step: uh-oh! found a document node somehow (%p)", (void *)each);
                evaluator->code = ERR_UNEXPECTED_DOCUMENT_NODE;
                result = false;
                break;
        }
    }

    return result;
}

bool apply_node_test(Node *each, void *argument, Nodelist *target)
{
    bool result = true;
    Evaluator *evaluator = (Evaluator *)argument;

    switch(current_step(evaluator)->test.kind)
    {
        case WILDCARD_TEST:
            if(RECURSIVE == current_step(evaluator)->kind)
            {
                result = apply_recursive_wildcard_test(each, argument, target);
            }
            else
            {
                result = apply_greedy_wildcard_test(each, argument, target);
            }
            break;
        case TYPE_TEST:
            result = apply_type_test(each, argument, target);
            break;
        case NAME_TEST:
            result = apply_name_test(each, argument, target);
            break;
    }

    return result;
}
