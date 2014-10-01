/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>.  All rights reserved.
 *
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README file in the project root.
 *
 * Distributed under an [MIT-style][license] license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimers.
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimers in the documentation and/or
 *   other materials provided with the distribution.
 * - Neither the names of the copyright holders, nor the names of the authors, nor
 *   the names of other contributors may be used to endorse or promote products
 *   derived from this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/ncsa
 */

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#ifdef __APPLE__
#define _DARWIN_SOURCE
#endif

#include <string.h>
#include <stdio.h>

#include "evaluator.h"
#include "evaluator/private.h"
#include "log.h"
#include "conditions.h"

struct meta_context
{
    evaluator_context *context;
    nodelist *target;
};

typedef struct meta_context meta_context;

static bool evaluate_step(step* each, void *context);
static bool evaluate_root_step(evaluator_context *context);
static bool evaluate_single_step(evaluator_context *context);
static bool evaluate_recursive_step(evaluator_context *context);
static bool evaluate_predicate(evaluator_context *context);

static bool apply_node_test(node *each, void *argument, nodelist *target);
static bool apply_recursive_node_test(node *each, void *argument, nodelist *target);
static bool recursive_test_sequence_iterator(node *each, void *context);
static bool recursive_test_map_iterator(node *key, node *value, void *context);
static bool apply_greedy_wildcard_test(node *each, void *argument, nodelist *target);
static bool apply_recursive_wildcard_test(node *each, void *argument, nodelist *target);
static bool apply_type_test(node *each, void *argument, nodelist *target);
static bool apply_name_test(node *each, void *argument, nodelist *target);

static bool apply_predicate(node *value, void *argument, nodelist *target);
static bool apply_wildcard_predicate(node *value, evaluator_context *context, nodelist *target);
static bool apply_subscript_predicate(node *value, evaluator_context *context, nodelist *target);
static bool apply_slice_predicate(node *value, evaluator_context *context, nodelist *target);
static bool apply_join_predicate(node *value, evaluator_context *context, nodelist *target);

static bool add_to_nodelist_sequence_iterator(node *each, void *context);
static bool add_values_to_nodelist_map_iterator(node *key, node *value, void *context);
static void normalize_interval(node *value, predicate *slice, int *from, int *to, int *step);

#define current_step(CONTEXT) path_get((CONTEXT)->path, (CONTEXT)->current_step)
#define guard(EXPR) EXPR ? true : (context->code = ERR_EVALUATOR_OUT_OF_MEMORY, false)


evaluator_status_code evaluate_steps(const document_model *model, const jsonpath *path, nodelist **list)
{
    evaluator_debug("beginning evaluation of %d steps", path_length(path));

    evaluator_context context;
    memset(&context, 0, sizeof(evaluator_context));

    *list = NULL;
    context.list = make_nodelist();
    if(NULL == context.list)
    {
        evaluator_debug("uh oh! out of memory, can't allocate the result nodelist");
        return ERR_EVALUATOR_OUT_OF_MEMORY;
    }

    context.model = model;
    context.path = path;

    nodelist_add(context.list, model_document(model, 0));

    if(!path_iterate(path, evaluate_step, &context))
    {
        evaluator_error("aborted, step: %d, code: %d (%s)", context.current_step, context.code, evaluator_status_message(context.code));
        return context.code;
    }

    evaluator_debug("done, found %d matching nodes", nodelist_length(context.list));
    *list = context.list;
    return context.code;
}

static bool evaluate_step(step* each, void *argument)
{
    evaluator_context *context = (evaluator_context *)argument;
    evaluator_trace("step: %zd", context->current_step);

    bool result = false;
    switch(step_kind(each))
    {
        case ROOT:
            result = evaluate_root_step(context);
            break;
        case SINGLE:
            result = evaluate_single_step(context);
            break;
        case RECURSIVE:
            result = evaluate_recursive_step(context);
            break;
    }
    if(result && step_has_predicate(current_step(context)))
    {
        result = evaluate_predicate(context);
    }
    if(result)
    {
        context->current_step++;
    }
    return result;
}

static bool evaluate_root_step(evaluator_context *context)
{
    evaluator_trace("evaluating root step");
    node *document = nodelist_get(context->list, 0);
    node *root = document_root(document);
    evaluator_trace("root test: adding root node (%p) from document (%p)", root, document);
    return nodelist_set(context->list, root, 0);
}

#define evaluate_nodelist(NAME, TEST, FUNCTION)                         \
    evaluator_trace("evaluating %s across %zd nodes", (NAME), nodelist_length(context->list)); \
    nodelist *result = nodelist_map(context->list, (FUNCTION), context); \
    evaluator_trace("%s: %s", (TEST), NULL == result ? "failed" : "completed"); \
    evaluator_trace("%s: added %zd nodes", (NAME), nodelist_length(result)); \
    return NULL == result ? false : (nodelist_free(context->list), context->list = result, true);

static bool evaluate_recursive_step(evaluator_context *context)
{
    evaluate_nodelist("recursive step",
                      test_kind_name(step_test_kind(current_step(context))),
                      apply_recursive_node_test);
}

static bool evaluate_single_step(evaluator_context *context)
{
    evaluate_nodelist("step",
                      test_kind_name(step_test_kind(current_step(context))),
                      apply_node_test);
}

static bool evaluate_predicate(evaluator_context *context)
{
    evaluate_nodelist("predicate",
                      predicate_kind_name(predicate_kind(step_predicate(current_step(context)))),
                      apply_predicate);
}

static bool apply_recursive_node_test(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    bool result = true;
    if(ALIAS != node_kind(each))
    {
        result = apply_node_test(each, argument, target);
    }
    if(result)
    {
        switch(node_kind(each))
        {
            case MAPPING:
                evaluator_trace("recursive step: processing %zd mapping values (%p)", node_size(each), each);
                result = mapping_iterate(each, recursive_test_map_iterator, &(meta_context){context, target});
                break;
            case SEQUENCE:
                evaluator_trace("recursive step: processing %zd sequence items (%p)", node_size(each), each);
                result = sequence_iterate(each, recursive_test_sequence_iterator, &(meta_context){context, target});
                break;
            case SCALAR:
                evaluator_trace("recursive step: found scalar, recursion finished on this path (%p)", each);
                result = true;
                break;
            case DOCUMENT:
                evaluator_error("recursive step: uh-oh! found a document node somehow (%p), aborting...", each);
                context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
                result = false;
                break;
            case ALIAS:
                evaluator_trace("recursive step: resolving alias (%p)", each);
                result = apply_recursive_node_test(alias_target(each), argument, target);
                break;
        }
    }

    return result;
}

static bool recursive_test_sequence_iterator(node *each, void *context)
{
    meta_context *iterator_context = (meta_context *)context;
    return apply_recursive_node_test(each, iterator_context->context, iterator_context->target);
}

static bool recursive_test_map_iterator(node *key __attribute__((unused)), node *value, void *context)
{
    meta_context *iterator_context = (meta_context *)context;
    return apply_recursive_node_test(value, iterator_context->context, iterator_context->target);
}

static bool apply_node_test(node *each, void *argument, nodelist *target)
{
    bool result = false;
    evaluator_context *context = (evaluator_context *)argument;
    switch(step_test_kind(current_step(context)))
    {
        case WILDCARD_TEST:
            if(RECURSIVE == step_kind(current_step(context)))
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

static bool apply_greedy_wildcard_test(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    bool result = false;
    switch(node_kind(each))
    {
        case MAPPING:
            evaluator_trace("wildcard test: adding %zd mapping values (%p)", node_size(each), each);
            result = guard(mapping_iterate(each, add_values_to_nodelist_map_iterator, &(meta_context){context, target}));
            break;
        case SEQUENCE:
            evaluator_trace("wildcard test: adding %zd sequence items (%p)", node_size(each), each);
            result = guard(sequence_iterate(each, add_to_nodelist_sequence_iterator, target));
            break;
        case SCALAR:
            trace_string("wildcard test: adding scalar: '%s' (%p)", scalar_value(each), node_size(each), each);
            result = guard(nodelist_add(target, each));
            break;
        case DOCUMENT:
            evaluator_error("wildcard test: uh-oh! found a document node somehow (%p), aborting...", each);
            context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
        case ALIAS:
            evaluator_trace("wildcard test: resolving alias (%p)", node_size(each), each);
            result = apply_greedy_wildcard_test(alias_target(each), argument, target);
            break;
    }
    return result;
}

static bool apply_recursive_wildcard_test(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    bool result = false;
    switch(node_kind(each))
    {
        case MAPPING:
            evaluator_trace("recurisve wildcard test: adding mapping node (%p)", each);
            result = guard(nodelist_add(target, each));
            break;
        case SEQUENCE:
            evaluator_trace("recurisve wildcard test: adding sequence node (%p)", each);
            result = guard(nodelist_add(target, each));
            break;
        case SCALAR:
            trace_string("recurisve wildcard test: adding scalar: '%s' (%p)", scalar_value(each), node_size(each), each);
            result = guard(nodelist_add(target, each));
            break;
        case DOCUMENT:
            evaluator_error("recurisve wildcard test: uh oh! found a document node somehow (%p), aborting...", each);
            context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
        case ALIAS:
            evaluator_trace("recurisve wildcard test: resolving alias (%p)", each);
            result = apply_recursive_wildcard_test(alias_target(each), argument, target);
            break;
    }
    return result;
}

static bool apply_type_test(node *each, void *argument, nodelist *target)
{
    bool match = false;
    node *value = each;
    if(ALIAS == node_kind(value))
    {
        evaluator_trace("type test: resolved alias from: (%p) to: (%p)", value, alias_target(value));
        value = alias_target(value);
    }
    evaluator_context *context = (evaluator_context *)argument;
    switch(type_test_step_kind(current_step(context)))
    {
        case OBJECT_TEST:
            evaluator_trace("type test: testing for an object");
            match = MAPPING == node_kind(value);
            break;
        case ARRAY_TEST:
            evaluator_trace("type test: testing for an array");
            match = SEQUENCE == node_kind(value);
            break;
        case STRING_TEST:
            evaluator_trace("type test: testing for a string");
            match = SCALAR == node_kind(value) && SCALAR_STRING == scalar_kind(value);
            break;
        case NUMBER_TEST:
            evaluator_trace("type test: testing for a number");
            match = SCALAR == node_kind(value) &&
                (SCALAR_INTEGER == scalar_kind(value) || SCALAR_REAL == scalar_kind(value));
            break;
        case BOOLEAN_TEST:
            evaluator_trace("type test: testing for a boolean");
            match = SCALAR == node_kind(value) && SCALAR_BOOLEAN == scalar_kind(value);
            break;
        case NULL_TEST:
            evaluator_trace("type test: testing for a null");
            match = SCALAR == node_kind(value) && SCALAR_NULL == scalar_kind(value);
            break;
    }
    if(match)
    {
        evaluator_trace("type test: match! adding node (%p)", value);
        return nodelist_add(target, value) ? true : (context->code = ERR_EVALUATOR_OUT_OF_MEMORY, false);
    }
    else
    {
        evaluator_trace("type test: no match (actual: %d). dropping (%p)", SCALAR == node_kind(value) ? scalar_kind(value) : node_kind(value), value);
        return true;
    }
}

static bool apply_name_test(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    step *context_step = current_step(context);
    trace_string("name test: using key '%s'", name_test_step_name(context_step), name_test_step_length(context_step));

    if(MAPPING != node_kind(each))
    {
        evaluator_trace("name test: node is not a mapping type, cannot use a key on it (kind: %d), dropping (%p)", node_kind(each), each);
        return true;
    }
    node *value = mapping_get(each, name_test_step_name(context_step), name_test_step_length(context_step));
    if(NULL == value)
    {
        evaluator_trace("name test: key not found in mapping, dropping (%p)", each);
        return true;
    }
    evaluator_trace("name test: match! adding node (%p)", value);
    if(ALIAS == node_kind(value))
    {
        evaluator_trace("name test: resolved alias from: (%p) to: (%p)", value, alias_target(value));
        value = alias_target(value);
    }
    return nodelist_add(target, value) ? true : (context->code = ERR_EVALUATOR_OUT_OF_MEMORY, false);
}

static bool apply_predicate(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    bool result = false;
    switch(predicate_kind(step_predicate(current_step(context))))
    {
        case WILDCARD:
            evaluator_trace("evaluating wildcard predicate");
            result = apply_wildcard_predicate(each, context, target);
            break;
        case SUBSCRIPT:
            evaluator_trace("evaluating subscript predicate");
            result = apply_subscript_predicate(each, context, target);
            break;
        case SLICE:
            evaluator_trace("evaluating slice predicate");
            result = apply_slice_predicate(each, context, target);
            break;
        case JOIN:
            evaluator_trace("evaluating join predicate");
            result = apply_join_predicate(each, context, target);
            break;
    }
    return result;
}

static bool apply_wildcard_predicate(node *value, evaluator_context *context, nodelist *target)
{
    bool result = false;
    switch(node_kind(value))
    {
        case SCALAR:
            trace_string("wildcard predicate: adding scalar '%s' (%p)", scalar_value(value), node_size(value), value);
            result = guard(nodelist_add(target, value));
            break;
        case MAPPING:
            evaluator_trace("wildcard predicate: adding mapping (%p)", value);
            result = guard(nodelist_add(target, value));
            break;
        case SEQUENCE:
            evaluator_trace("wildcard predicate: adding %zd sequence (%p) items", node_size(value), value);
            result = guard(sequence_iterate(value, add_to_nodelist_sequence_iterator, target));
            break;
        case DOCUMENT:
            evaluator_error("wildcard predicate: uh-oh! found a document node somehow (%p), aborting...", value);
            context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
        case ALIAS:
            evaluator_trace("wildcard predicate: resolving alias (%p)", value);
            result = apply_wildcard_predicate(alias_target(value), context, target);
            break;
    }
    return result;
}

static bool apply_subscript_predicate(node *value, evaluator_context *context, nodelist *target)
{
    if(SEQUENCE != node_kind(value))
    {
        evaluator_trace("subscript predicate: node is not a sequence type, cannot use an index on it (kind: %d), dropping (%p)", node_kind(value), value);
        return true;
    }
    predicate *subscript = step_predicate(current_step(context));
    size_t index = subscript_predicate_index(subscript);
    if(index > node_size(value))
    {
        evaluator_trace("subscript predicate: index %zd not valid for sequence (length: %zd), dropping (%p)", index, node_size(value), value);
        return true;
    }
    node *selected = sequence_get(value, index);
    evaluator_trace("subscript predicate: adding index %zd (%p) from sequence (%p) of %zd items", index, selected, value, node_size(value));
    return nodelist_add(target, selected) ? true : (context->code = ERR_EVALUATOR_OUT_OF_MEMORY, false);
}

static bool apply_slice_predicate(node *value, evaluator_context *context, nodelist *target)
{
    if(SEQUENCE != node_kind(value))
    {
        evaluator_trace("slice predicate: node is not a sequence type, cannot use a slice on it (kind: %d), dropping (%p)", node_kind(value), value);
        return true;
    }

    predicate *slice = step_predicate(current_step(context));
    int from = 0, to = 0, increment = 0;
    normalize_interval(value, slice, &from, &to, &increment);
    evaluator_trace("slice predicate: using normalized interval [%d:%d:%d]", from, to, increment);

    for(int i = from; 0 > increment ? i >= to : i < to; i += increment)
    {
        node *selected = sequence_get(value, (size_t)i);
        if(NULL == selected || !nodelist_add(target, selected))
        {
            evaluator_error("slice predicate: uh oh! out of memory, aborting. index: %d, selected: %p", i, selected);
            context->code = ERR_EVALUATOR_OUT_OF_MEMORY;
            return false;
        }
        evaluator_trace("slice predicate: adding index: %d (%p)", i, selected);
    }
    return true;
}

static bool apply_join_predicate(node *value __attribute__((unused)), evaluator_context *context, nodelist *target __attribute__((unused)))
{
    evaluator_trace("join predicate: evaluating axes (_, _)");

    // xxx - implement me!
    evaluator_error("join predicate: uh-oh! not implemented yet, aborting...");
    context->code = ERR_UNSUPPORTED_PATH;
    return false;
}

/*
 * Utility Functions
 * =================
 */

static bool add_to_nodelist_sequence_iterator(node *each, void *context)
{
    nodelist *list = (nodelist *)context;
    node *value = each;
    if(ALIAS == node_kind(each))
    {
        value = alias_target(each);
    }
    return nodelist_add(list, value);
}

static bool add_values_to_nodelist_map_iterator(node *key __attribute__((unused)), node *value, void *context)
{
    meta_context *iterator_context = (meta_context *)context;
    bool result = false;
    switch(node_kind(value))
    {
        case SCALAR:
            trace_string("wildcard test: adding scalar mapping value: '%s' (%p)", scalar_value(value), node_size(value), value);
            result = nodelist_add(iterator_context->target, value);
            break;
        case MAPPING:
            evaluator_trace("wildcard test: adding mapping mapping value (%p)", value);
            result = nodelist_add(iterator_context->target, value);
            break;
        case SEQUENCE:
            evaluator_trace("wildcard test: adding %zd sequence mapping values (%p) items", node_size(value), value);
            result = sequence_iterate(value, add_to_nodelist_sequence_iterator, iterator_context->target);
            break;
        case DOCUMENT:
            evaluator_error("wildcard test: uh-oh! found a document node somehow (%p), aborting...", value);
            iterator_context->context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            result = false;
            break;
        case ALIAS:
            evaluator_trace("wildcard test: resolving alias (%p)", value);
            result = add_values_to_nodelist_map_iterator(key, alias_target(value), context);
            break;
    }
    return result;
}

static int normalize_extent(bool specified_p, int given, int fallback, int limit)
{
    if(!specified_p)
    {
        evaluator_trace("slice predicate: (normalizer) no value specified, defaulting to %zd", fallback);
        return fallback;
    }
    int result = 0 > given ? given + limit: given;
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

static int normalize_from(predicate *slice, node *value)
{
    evaluator_trace("slice predicate: normalizing from, specified: %s, value: %d", slice_predicate_has_from(slice) ? "yes" : "no", slice_predicate_from(slice));
    int length = (int)node_size(value);
    return normalize_extent(slice_predicate_has_from(slice), (int)slice_predicate_from(slice), 0, length);
}

static int normalize_to(predicate *slice, node *value)
{
    evaluator_trace("slice predicate: normalizing to, specified: %s, value: %d", slice_predicate_has_to(slice) ? "yes" : "no", slice_predicate_to(slice));
    int length = (int)node_size(value);
    return normalize_extent(slice_predicate_has_to(slice), (int)slice_predicate_to(slice), length, length);
}

static void normalize_interval(node *value, predicate *slice, int *from, int *to, int *increment)
{
#ifdef USE_LOGGING
    char *from_fmt = NULL, *to_fmt = NULL, *increment_fmt = NULL;
    int from_result  = 0;
    int to_result = 0;
    int inc_result = 0;
    evaluator_trace("slice predicate: evaluating interval [%s:%s:%s] on sequence (%p) of %zd items",
                    slice_predicate_has_from(slice) ? (from_result = asprintf(&from_fmt, "%d", (int)slice_predicate_from(slice)), -1 == from_result ? "?" : from_fmt) : "_",
                    slice_predicate_has_to(slice) ? (to_result = asprintf(&to_fmt, "%d", (int)slice_predicate_to(slice)), -1 == to_result ? "?" : to_fmt) : "_",
                    slice_predicate_has_step(slice) ? (inc_result = asprintf(&increment_fmt, "%d", (int)slice_predicate_step(slice)), -1 == inc_result ? "?" : increment_fmt) : "_",
                    value, node_size(value));
    free(from_fmt); free(to_fmt); free(increment_fmt);
#endif
    *increment = slice_predicate_has_step(slice) ? (int)slice_predicate_step(slice) : 1;
    *from = 0 > *increment ? normalize_to(slice, value) - 1 : normalize_from(slice, value);
    *to   = 0 > *increment ? normalize_from(slice, value) : normalize_to(slice, value);
}
