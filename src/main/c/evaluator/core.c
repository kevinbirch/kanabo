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

static nodelist *evaluate_steps(evaluator_context *context);
static bool evaluate_step(step* each, void *context);
static bool evaluate_root_step(evaluator_context *context);
static bool evaluate_single_step(evaluator_context *context);
static bool evaluate_recursive_step(evaluator_context *context);
static bool evaluate_predicate(evaluator_context *context);
static inline bool evaluate_nodelist(evaluator_context *context, const char *name, const char *test, nodelist_map_function function);

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

static bool add_values_to_nodelist_map_iterator(node *key, node *value, void *context);
static void normalize_interval(node *value, predicate *slice, int_fast32_t *from, int_fast32_t *to, int_fast32_t *step);
static int_fast32_t normalize_from(predicate *predicate, node *value);
static int_fast32_t normalize_to(predicate *predicate, node *value);
static int_fast32_t normalize_extent(bool specified_p, int_fast32_t actual, int_fast32_t fallback, int_fast32_t length);

#define current_step(CONTEXT) path_get((CONTEXT)->path, (CONTEXT)->current_step)
#define guard(EXPR) EXPR ? true : (evaluator_error("uh oh! out of memory, aborting. (line: " S(__LINE__) ")"), context->code = ERR_EVALUATOR_OUT_OF_MEMORY, false)

nodelist *evaluate(evaluator_context *context)
{
    PRECOND_NONNULL_ELSE_NULL(context);
    PRECOND_NONNULL_ELSE_NULL(context->list);
    PRECOND_NONNULL_ELSE_NULL(context->model);
    PRECOND_NONNULL_ELSE_NULL(context->path);
    PRECOND_NONNULL_ELSE_NULL(model_get_document(context->model, 0));
    PRECOND_NONNULL_ELSE_NULL(model_get_document_root(context->model, 0));
    PRECOND_ELSE_NULL(ABSOLUTE_PATH == path_kind(context->path));
    PRECOND_ELSE_NULL(0 != path_length(context->path));

    nodelist_add(context->list, model_get_document(context->model, 0));
    
    return evaluate_steps(context);
}

static nodelist *evaluate_steps(evaluator_context *context)
{
    evaluator_debug("beginning evaluation of %d steps", path_length(context->path));

    if(!path_iterate(context->path, evaluate_step, context))
    {
        evaluator_debug("aborted, step: %d, code: %d (%s)", context->current_step, context->code, evaluator_status_message(context));
        nodelist_free(context->list);
        context->list = NULL;
        return NULL;
    }

    evaluator_debug("done, found %d matching nodes", nodelist_length(context->list));
    return context->list;
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
    node *root = document_get_root(document);
    evaluator_trace("root test: adding root node (%p) from document (%p)", root, document);
    return nodelist_set(context->list, root, 0);
}

static bool evaluate_recursive_step(evaluator_context *context)
{
    return evaluate_nodelist(context, "recursive step", test_kind_name(step_test_kind(current_step(context))), apply_recursive_node_test);
}

static bool evaluate_single_step(evaluator_context *context)
{
    return evaluate_nodelist(context, "single step", test_kind_name(step_test_kind(current_step(context))), apply_node_test);
}

static bool evaluate_predicate(evaluator_context *context)
{
    return evaluate_nodelist(context, "predicate", predicate_kind_name(predicate_kind(step_predicate(current_step(context)))), apply_predicate);
}

static inline bool evaluate_nodelist(evaluator_context *context, const char *name, const char *test, nodelist_map_function function)
{
    evaluator_trace("evaluating %s across %zd nodes", name, nodelist_length(context->list));
    nodelist *result = nodelist_map(context->list, function, context);
    evaluator_trace("%s: %s", test, NULL == result ? "failed" : "completed");
    evaluator_trace("%s: added %zd nodes", name, nodelist_length(result));
    return NULL == result ? false : (nodelist_free(context->list), context->list = result, true);
}

static bool apply_recursive_node_test(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    bool result = apply_node_test(each, argument, target);
    if(result)
    {
        switch(node_get_kind(each))
        {
            case MAPPING:
                evaluator_trace("recursive step: processing %zd mapping values (%p)", node_get_size(each), each);
                result = iterate_mapping(each, recursive_test_map_iterator, &(meta_context){context, target});
                break;
            case SEQUENCE:
                evaluator_trace("recursive step: processing %zd sequence items (%p)", node_get_size(each), each);
                result = iterate_sequence(each, recursive_test_sequence_iterator, &(meta_context){context, target});
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
#pragma unused(key)
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
    switch(node_get_kind(each))
    {
        case MAPPING:
            evaluator_trace("wildcard test: adding %zd mapping values (%p)", node_get_size(each), each);
            result = guard(iterate_mapping(each, add_values_to_nodelist_map_iterator, &(meta_context){context, target}));
            break;
        case SEQUENCE:
            evaluator_trace("wildcard test: adding %zd sequence items (%p)", node_get_size(each), each);
            result = guard(iterate_sequence(each, add_to_nodelist_sequence_iterator, target));
            break;
        case SCALAR:
            trace_string("wildcard test: adding scalar: '%s' (%p)", scalar_get_value(each), node_get_size(each), each);
            result = guard(nodelist_add(target, each));
            break;
        case DOCUMENT:
            evaluator_trace("wildcard test: uh-oh! found a document node somehow (%p), aborting...", each);
            context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
    }
    return result;
}

static bool apply_recursive_wildcard_test(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    bool result = false;
    switch(node_get_kind(each))
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
            trace_string("recurisve wildcard test: adding scalar: '%s' (%p)", scalar_get_value(each), node_get_size(each), each);
            result = guard(nodelist_add(target, each));
            break;
        case DOCUMENT:
            evaluator_trace("recurisve wildcard test: uh-oh! found a document node somehow (%p), aborting...", each);
            context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
    }
    return result;
}

static bool apply_type_test(node *each, void *argument, nodelist *target)
{
    bool match = false;
    evaluator_context *context = (evaluator_context *)argument;
    switch(type_test_step_kind(current_step(context)))
    {
        case OBJECT_TEST:
            evaluator_trace("type test: testing for an object (%d)", MAPPING);
            match = MAPPING == node_get_kind(each);
            break;
        case ARRAY_TEST:
            evaluator_trace("type test: testing for an array (%d)", SEQUENCE);
            match = SEQUENCE == node_get_kind(each);
            break;
        case STRING_TEST:
            evaluator_trace("type test: testing for a string (%d)", SCALAR_STRING);
            match = SCALAR == node_get_kind(each) && SCALAR_STRING == scalar_get_kind(each);
            break;
        case NUMBER_TEST:
            evaluator_trace("type test: testing for a number (%d)", SCALAR_NUMBER);
            match = SCALAR == node_get_kind(each) && SCALAR_NUMBER == scalar_get_kind(each);
            break;
        case BOOLEAN_TEST:
            evaluator_trace("type test: testing for a boolean (%d)", SCALAR_BOOLEAN);
            match = SCALAR == node_get_kind(each) && SCALAR_BOOLEAN == scalar_get_kind(each);
            break;
        case NULL_TEST:
            evaluator_trace("type test: testing for a null (%d)", SCALAR_NULL);
            match = SCALAR == node_get_kind(each) && SCALAR_NULL == scalar_get_kind(each);
            break;
    }
    if(match)
    {        
        evaluator_trace("type test: match! adding node (%p)", each);
        return nodelist_add(target, each) ? true : (context->code = ERR_EVALUATOR_OUT_OF_MEMORY, false);
    }
    else
    {
        evaluator_trace("type test: no match (actual: %d). dropping (%p)", SCALAR == node_get_kind(each) ? scalar_get_kind(each) : node_get_kind(each), each);
        return true;
    }
}

static bool apply_name_test(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    step *context_step = current_step(context);
    trace_string("name test: using key '%s'", name_test_step_name(context_step), name_test_step_length(context_step));

    if(MAPPING != node_get_kind(each))
    {
        evaluator_trace("name test: node is not a mapping type, cannot use a key on it (kind: %d), dropping (%p)", node_get_kind(each), each);
        return true;
    }
    node *value = mapping_get_value_scalar_key(each, name_test_step_name(context_step), name_test_step_length(context_step));
    if(NULL == value)
    {
        evaluator_trace("name test: key not found in mapping, dropping (%p)", each);
        return true;
    }
    evaluator_trace("name test: match! adding node (%p)", value);
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
    switch(node_get_kind(value))
    {
        case SCALAR:
            trace_string("wildcard predicate: adding scalar '%s' (%p)", scalar_get_value(value), node_get_size(value), value);
            result = guard(nodelist_add(target, value));
            break;
        case MAPPING:
            evaluator_trace("wildcard predicate: adding mapping (%p)", value);
            result = guard(nodelist_add(target, value));
            break;
        case SEQUENCE:
            evaluator_trace("wildcard predicate: adding %zd sequence (%p) items", node_get_size(value), value);
            result = guard(iterate_sequence(value, add_to_nodelist_sequence_iterator, target));
            break;
        case DOCUMENT:
            evaluator_error("wildcard predicate: uh-oh! found a document node somehow (%p), aborting...", value);
            context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            break;
    }
    return result;
}

static bool apply_subscript_predicate(node *value, evaluator_context *context, nodelist *target)
{
    if(SEQUENCE != node_get_kind(value))
    {
        evaluator_trace("subscript predicate: node is not a sequence type, cannot use an index on it (kind: %d), dropping (%p)", node_get_kind(value), value);
    }
    predicate *subscript = step_predicate(current_step(context));
    size_t index = subscript_predicate_index(subscript);
    if(index > node_get_size(value))
    {
        evaluator_trace("subscript predicate: index %zd not valid for sequence (length: %zd), dropping (%p)", index, node_get_size(value), value);
        return true;
    }    
    node *selected = sequence_get(value, index);
    evaluator_trace("subscript predicate: adding index %zd (%p) from sequence (%p) of %zd items", index, selected, value, node_get_size(value));
    return nodelist_add(target, selected) ? true : (context->code = ERR_EVALUATOR_OUT_OF_MEMORY, false);
}

static bool apply_slice_predicate(node *value, evaluator_context *context, nodelist *target)
{
    if(SEQUENCE != node_get_kind(value))
    {
        evaluator_trace("slice predicate: node is not a sequence type, cannot use a slice on it (kind: %d), dropping (%p)", node_get_kind(value), value);
    }

    predicate *slice = step_predicate(current_step(context));
    int_fast32_t from = 0, to = 0, increment = 0;
    normalize_interval(value, slice, &from, &to, &increment);
    evaluator_trace("slice predicate: using normalized interval [%d:%d:%d]", from, to, increment);

    for(int_fast32_t i = from; 0 > increment ? i >= to : i < to; i += increment)
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
#pragma unused(value, target)
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

static bool add_values_to_nodelist_map_iterator(node *key __attribute__((unused)), node *value, void *context)
{
#pragma unused(key)
    meta_context *iterator_context = (meta_context *)context;
    bool result = false;
    switch(node_get_kind(value))
    {
        case SCALAR:
            trace_string("wildcard test: adding scalar mapping value: '%s' (%p)", scalar_get_value(value), node_get_size(value), value);
            result = nodelist_add(iterator_context->target, value);
            break;
        case MAPPING:
            evaluator_trace("wildcard test: adding mapping mapping value (%p)", value);
            result = nodelist_add(iterator_context->target, value);
            break;
        case SEQUENCE:
            evaluator_trace("wildcard test: adding %zd sequence mapping values (%p) items", node_get_size(value), value);
            result = iterate_sequence(value, add_to_nodelist_sequence_iterator, iterator_context->target);
            break;
        case DOCUMENT:
            evaluator_error("wildcard test: uh-oh! found a document node somehow (%p), aborting...", value);
            iterator_context->context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            result = false;
            break;
    }
    return result;
}

static void normalize_interval(node *value, predicate *slice, int_fast32_t *from, int_fast32_t *to, int_fast32_t *increment)
{
    char *from_fmt = NULL, *to_fmt = NULL, *increment_fmt = NULL;
    int from_result = 0, to_result = 0, inc_result = 0;
    evaluator_trace("slice predicate: evaluating interval [%s:%s:%s] on sequence (%p) of %zd items",
                    slice_predicate_has_from(slice) ? (from_result = asprintf(&from_fmt, "%zd", slice_predicate_from(slice)), -1 == from_result ? "?" : from_fmt) : "_",
                    slice_predicate_has_to(slice) ? (to_result = asprintf(&to_fmt, "%zd", slice_predicate_to(slice)), -1 == to_result ? "?" : to_fmt) : "_",
                    slice_predicate_has_step(slice) ? (inc_result = asprintf(&increment_fmt, "%zd", slice_predicate_step(slice)), -1 == inc_result ? "?" : increment_fmt) : "_",
                    value, node_get_size(value));
    free(from_fmt); free(to_fmt); free(increment_fmt);
    *increment = slice_predicate_has_step(slice) ? slice_predicate_step(slice) : 1;
    *from = 0 > *increment ? normalize_to(slice, value) - 1 : normalize_from(slice, value);
    *to   = 0 > *increment ? normalize_from(slice, value) : normalize_to(slice, value);
}

static int_fast32_t normalize_from(predicate *slice, node *value)
{
    evaluator_trace("slice predicate: normalizing from, specified: %s, value: %d", slice_predicate_has_from(slice) ? "yes" : "no", slice_predicate_from(slice));
    int_fast32_t length = (int_fast32_t)node_get_size(value);
    return normalize_extent(slice_predicate_has_from(slice), slice_predicate_from(slice), 0, length);
}

static int_fast32_t normalize_to(predicate *slice, node *value)
{
    evaluator_trace("slice predicate: normalizing to, specified: %s, value: %d", slice_predicate_has_to(slice) ? "yes" : "no", slice_predicate_to(slice));
    int_fast32_t length = (int_fast32_t)node_get_size(value);
    return normalize_extent(slice_predicate_has_to(slice), slice_predicate_to(slice), length, length);
}

static int_fast32_t normalize_extent(bool specified_p, int_fast32_t given, int_fast32_t fallback, int_fast32_t limit)
{
    if(!specified_p)
    {
        evaluator_trace("slice predicate: (normalizer) no value specified, defaulting to %d", fallback);
        return fallback;
    }
    int_fast32_t result = 0 > given ? given + limit: given;
    if(0 > result)
    {
        evaluator_trace("slice predicate: (normalizer) negative value, clamping to zero");
        return 0;
    }
    if(limit < result)
    {
        evaluator_trace("slice predicate: (normalizer) value over limit, clamping to %d", limit);
        return limit;
    }
    evaluator_trace("slice predicate: (normalizer) constrained to %d", result);
    return result;
}

