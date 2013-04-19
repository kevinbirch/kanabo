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

#include <errno.h>
#include <string.h>

#include "evaluator.h"
#include "conditions.h"
#include "log.h"

static nodelist *evaluate_steps(document_model *model, jsonpath *path);
static bool evaluate_step(step *current, nodelist **list);
static bool evaluate_root_step(nodelist **list);
static bool evaluate_single_step(step *current, nodelist **list);
static bool evaluate_recursive_step(step *current, nodelist **list);

static inline bool evaluate_test(const char * restrict name, step *current, nodelist **list, nodelist_map_function function);

static bool apply_wildcard_test(node *each, void *context, nodelist *target);
static bool apply_type_test(node *each, void *context, nodelist *target);
static bool apply_name_test(node *each, void *context, nodelist *target);

static bool evaluate_predicate(node *value, predicate *context, nodelist *target);
static bool apply_wildcard_predicate(node *value, predicate *wildcard, nodelist *target);
static bool apply_subscript_predicate(node *value, predicate *subscript, nodelist *target);
static bool apply_slice_predicate(node *value, predicate *slice, nodelist *target);
static bool apply_join_predicate(node *value, predicate *join, nodelist *target);

static bool add_values_to_nodelist_map_iterator(node *key, node *value, void *context);
static void normalize_interval(node *value, predicate *slice, int_fast32_t *from, int_fast32_t *to, int_fast32_t *step);
static int_fast32_t normalize_from(predicate *predicate, node *value);
static int_fast32_t normalize_to(predicate *predicate, node *value);
static int_fast32_t normalize_extent(bool specified, int_fast32_t actual, int_fast32_t fallback, int_fast32_t length);
static nodelist *make_result_nodelist(document_model *model);

#define component_name "evaluator"

#define evaluator_info(FORMAT, ...)  log_info(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_debug(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_trace(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, ...) log_string(TRACE, component_name, FORMAT, ##__VA_ARGS__)

nodelist *evaluate(document_model *model, jsonpath *path)
{
    PRECOND_NONNULL_ELSE_NULL(model, path, model_get_document_root(model, 0));
    PRECOND_ELSE_NULL(ABSOLUTE_PATH == path_get_kind(path), 0 < path_get_length(path));

    evaluator_debug("beginning evaluation of %d steps", path_get_length(path));
    return evaluate_steps(model, path);
}

static nodelist *evaluate_steps(document_model *model, jsonpath *path)
{
    nodelist *list = make_result_nodelist(model);
    ENSURE_NONNULL_ELSE_NULL(errno, list);

    for(size_t i = 0; i < path_get_length(path); i++)
    {
        evaluator_trace("step: %zd", i);
        step *current = path_get_step(path, i);
        if(!evaluate_step(current, &list))
        {
            evaluator_trace("aborted");
            nodelist_free_nodes(list);
            nodelist_free(list);
            return NULL;
        }
    }

    evaluator_trace("done");
    return list;
}

static bool evaluate_step(step *current, nodelist **list)
{
    bool result = false;
    switch(step_get_kind(current))
    {
        case ROOT:
            result = evaluate_root_step(list);
            break;
        case SINGLE:
            result = evaluate_single_step(current, list);
            break;
        case RECURSIVE:
            result = evaluate_recursive_step(current, list);
            break;
    }
    return result;
}

static bool evaluate_root_step(nodelist **list)
{
    evaluator_trace("evaluating root step");
    node *document = nodelist_get(*list, 0);
    node *root = document_get_root(document);
    evaluator_trace("root test: adding root node (%p) from document (%p)", root, document);
    return nodelist_set(*list, root, 0);
}

static bool evaluate_recursive_step(step *current, nodelist **list)
{
#pragma unused(current)
    evaluator_trace("evaluating recursive step across %zd nodes", nodelist_length(*list));

    // xxx - implement me!
    evaluator_trace("recurisve step: uh oh! not implemented yet, aborting...");
    return false;
}

static bool evaluate_single_step(step *current, nodelist **list)
{
    bool result = false;
    evaluator_trace("evaluating single step across %zd nodes", nodelist_length(*list));
    switch(step_get_test_kind(current))
    {
        case WILDCARD_TEST:
            result = evaluate_test("wildcard", current, list, apply_wildcard_test);
            break;
        case TYPE_TEST:
            result = evaluate_test("type", current, list, apply_type_test);
            break;
        case NAME_TEST:
            result = evaluate_test("name", current, list, apply_name_test);
            break;
    }
    return result;
}

static inline bool evaluate_test(const char * restrict name, step *current, nodelist **list, nodelist_map_function function)
{
    evaluator_trace("evaluating %s test across %zd nodes", name, nodelist_length(*list));
    nodelist *result = nodelist_map(*list, function, current);
    evaluator_trace("%s test: test %s completed", name, NULL == result ? "was not" : "was");
    evaluator_trace("%s test: added %zd nodes", name, nodelist_length(result));
    return NULL == result ? false : (nodelist_free(*list), *list = result, true);
}

static bool apply_wildcard_test(node *each, void *context, nodelist *target)
{
#pragma unused(context)
    switch(node_get_kind(each))
    {
        case MAPPING:
            evaluator_trace("wildcard test: adding %zd mapping (%p) value sets", node_get_size(each), each);
            return iterate_mapping(each, add_values_to_nodelist_map_iterator, target);
        case SEQUENCE:
            evaluator_trace("wildcard test: adding %zd sequence (%p) items", node_get_size(each), each);
            return iterate_sequence(each, add_to_nodelist_sequence_iterator, target);
        case SCALAR:
            trace_string("wildcard test: adding scalar: '%s' (%p)", scalar_get_value(each), node_get_size(each), each);
            return nodelist_add(target, each);
        case DOCUMENT:
            evaluator_trace("wildcard test: uh-oh! found a document node somehow, aborting...");
            errno = ERR_MISPLACED_DOCUMENT_NODE;
            return false;
    }
}

static bool apply_type_test(node *each, void *context, nodelist *target)
{
    step *step_context = (step *)context;
    bool result = false;
    switch(type_test_step_get_type(step_context))
    {
        case OBJECT_TEST:
            evaluator_trace("type test: testing for an object (%d)", MAPPING);
            result = MAPPING == node_get_kind(each);
            break;
        case ARRAY_TEST:
            evaluator_trace("type test: testing for an array (%d)", SEQUENCE);
            result = SEQUENCE == node_get_kind(each);
            break;
        case STRING_TEST:
            evaluator_trace("type test: testing for a string (%d)", SCALAR_STRING);
            result = SCALAR == node_get_kind(each) && SCALAR_STRING == scalar_get_kind(each);
            break;
        case NUMBER_TEST:
            evaluator_trace("type test: testing for a number (%d)", SCALAR_NUMBER);
            result = SCALAR == node_get_kind(each) && SCALAR_NUMBER == scalar_get_kind(each);
            break;
        case BOOLEAN_TEST:
            evaluator_trace("type test: testing for a boolean (%d)", SCALAR_BOOLEAN);
            result = SCALAR == node_get_kind(each) && SCALAR_BOOLEAN == scalar_get_kind(each);
            break;
        case NULL_TEST:
            evaluator_trace("type test: testing for a null (%d)", SCALAR_NULL);
            result = SCALAR == node_get_kind(each) && SCALAR_NULL == scalar_get_kind(each);
            break;
    }
    if(result)
    {        
        evaluator_trace("type test: match! adding node (%p)", each);
        return nodelist_add(target, each);
    }
    else
    {
        evaluator_trace("type test: no match (actual: %d). dropping (%p)", SCALAR == node_get_kind(each) ? scalar_get_kind(each) : node_get_kind(each), each);
        return true;
    }
}

static bool apply_name_test(node *each, void *context, nodelist *target)
{
    step *step_context = (step *)context;
    trace_string("name test: using key '%s'", name_test_step_get_name(step_context), name_test_step_get_length(step_context));

    if(MAPPING != node_get_kind(each))
    {
        evaluator_trace("name test: node (%p) is not a mapping type, cannot use a key on it (kind: %d), dropping", each, node_get_kind(each));
        return true;
    }
    node *value = mapping_get_value_scalar_key(each, name_test_step_get_name(step_context), name_test_step_get_length(step_context));
    if(NULL == value)
    {
        evaluator_trace("name test: key not found in mapping (%p), dropping", each);
        return true;
    }
    if(step_has_predicate(step_context))
    {
        evaluator_trace("name test: match! evaluating predicate");
        bool result = evaluate_predicate(value, step_get_predicate(step_context), target);
        evaluator_trace("name test: predicate %s completed", result ? "was" : "was not");
        evaluator_trace("name test: (predicate) added %zd nodes", nodelist_length(target));
        return result;
    }
    else
    {
        evaluator_trace("name test: match! adding node (%p)", value);
        return nodelist_add(target, value);
    }
}

static bool evaluate_predicate(node *value, predicate *context, nodelist *target)
{
    switch(predicate_get_kind(context))
    {
        case WILDCARD:
            evaluator_trace("name test: evaluating wildcard predicate");
            return apply_wildcard_predicate(value, context, target);
        case SUBSCRIPT:
            evaluator_trace("name test: evaluating subscript predicate");
            return apply_subscript_predicate(value, context, target);
        case SLICE:
            evaluator_trace("name test: evaluating slice predicate");
            return apply_slice_predicate(value, context, target);
        case JOIN:
            evaluator_trace("name test: evaluating join predicate");
            return apply_join_predicate(value, context, target);
    }
}

static bool apply_wildcard_predicate(node *value, predicate *context, nodelist *target)
{
#pragma unused(context)
    switch(node_get_kind(value))
    {
        case SCALAR:
            trace_string("wildcard predicate: adding scalar '%s' (%p)", scalar_get_value(value), node_get_size(value), value);
            return nodelist_add(target, value);
        case MAPPING:
            evaluator_trace("wildcard predicate: adding mapping (%p)", value);
            return nodelist_add(target, value);
        case SEQUENCE:
            evaluator_trace("wildcard predicate: adding %zd sequence (%p) items", node_get_size(value), value);
            return iterate_sequence(value, add_to_nodelist_sequence_iterator, target);
        case DOCUMENT:
            evaluator_trace("wildcard predicate: uh-oh! found a document node somehow, aborting...");
            errno = ERR_MISPLACED_DOCUMENT_NODE;
            return false;
    }
}

static bool apply_subscript_predicate(node *value, predicate *subscript, nodelist *target)
{
    if(SEQUENCE != node_get_kind(value))
    {
        evaluator_trace("subscript predicate: node (%p) is not a sequence type, cannot use an index on it (kind: %d), dropping", value, node_get_kind(value));
    }
    size_t index = subscript_predicate_get_index(subscript);
    if(index > node_get_size(value))
    {
        evaluator_trace("subscript predicate: index %zd not valid for sequence (id: %p, length: %zd), dropping", index, value, node_get_size(value));
        return true;
    }    
    node *selected = sequence_get(value, index);
    evaluator_trace("subscript predicate: adding index %zd (%p) from sequence (%p) of %zd items", index, selected, value, node_get_size(value));
    return nodelist_add(target, selected);
}

static bool apply_slice_predicate(node *value, predicate *slice, nodelist *target)
{
    if(SEQUENCE != node_get_kind(value))
    {
        evaluator_trace("slice predicate: node (%p) is not a sequence type, cannot use a slice on it (kind: %d), dropping", value, node_get_kind(value));
    }

    int_fast32_t from = 0, to = 0, step = 0;
    normalize_interval(value, slice, &from, &to, &step);
    evaluator_trace("slice predicate: using normalized interval [%d:%d:%d]", from, to, step);

    for(int_fast32_t i = from; 0 > step ? i >= to : i < to; i += step)
    {
        errno = 0;
        node *selected = sequence_get(value, (size_t)i);
        if(NULL == selected || 0 != errno || !nodelist_add(target, selected))
        {
            evaluator_trace("slice predicate: uh oh! aborting. index: %d, selected: %p, errno: %d (\"%s\")", i, selected, errno, strerror(errno));
            return false;
        }
        evaluator_trace("slice predicate: adding index: %d (%p)", i, selected);         
    }
    return true;
}

static bool apply_join_predicate(node *value, predicate *join, nodelist *target)
{
#pragma unused(value, join, target)
    evaluator_trace("join predicate: evaluating axes (_, _)");

    // xxx - implement me!
    evaluator_trace("join predicate: uh-oh! not implemented yet, aborting...");
    return false;
}

/* 
 * Utility Functions
 * =================
 */

static bool add_values_to_nodelist_map_iterator(node *key, node *value, void *context)
{
#pragma unused(key)
    nodelist *target = (nodelist *)context;
    bool result = false;
    switch(node_get_kind(value))
    {
        case SCALAR:
        case MAPPING:
            result = nodelist_add(target, value);
            break;
        case SEQUENCE:
            result = iterate_sequence(value, add_to_nodelist_sequence_iterator, target);
            break;
        case DOCUMENT:
            errno = ERR_MISPLACED_DOCUMENT_NODE;
            result = false;
            break;
    }
    return result;
}

static void normalize_interval(node *value, predicate *slice, int_fast32_t *from, int_fast32_t *to, int_fast32_t *step)
{
    char *from_fmt = NULL, *to_fmt = NULL, *step_fmt = NULL;
    evaluator_trace("slice predicate: evaluating interval [%s:%s:%s] on sequence (%p) of %zd items",
                    slice_predicate_has_from(slice) ? (asprintf(&from_fmt, "%d", slice_predicate_get_from(slice)), from_fmt) : "_",
                    slice_predicate_has_to(slice) ? (asprintf(&to_fmt, "%d", slice_predicate_get_to(slice)), to_fmt) : "_",
                    slice_predicate_has_step(slice) ? (asprintf(&step_fmt, "%d", slice_predicate_get_step(slice)), step_fmt) : "_",
                    value, node_get_size(value));
    free(from_fmt); free(to_fmt); free(step_fmt);
    *step = slice_predicate_has_step(slice) ? slice_predicate_get_step(slice) : 1;
    *from = 0 > *step ? normalize_to(slice, value) - 1 : normalize_from(slice, value);
    *to   = 0 > *step ? normalize_from(slice, value) : normalize_to(slice, value);
}

static int_fast32_t normalize_from(predicate *slice, node *value)
{
    evaluator_trace("slice predicate: normalizing from, specified: %s, value: %d", slice_predicate_has_from(slice) ? "yes" : "no", slice_predicate_get_from(slice));
    int_fast32_t length = (int_fast32_t)node_get_size(value);
    return normalize_extent(slice_predicate_has_from(slice), slice_predicate_get_from(slice), 0, length);
}

static int_fast32_t normalize_to(predicate *slice, node *value)
{
    evaluator_trace("slice predicate: normalizing to, specified: %s, value: %d", slice_predicate_has_to(slice) ? "yes" : "no", slice_predicate_get_to(slice));
    int_fast32_t length = (int_fast32_t)node_get_size(value);
    return normalize_extent(slice_predicate_has_to(slice), slice_predicate_get_to(slice), length, length);
}

static int_fast32_t normalize_extent(bool specified, int_fast32_t given, int_fast32_t fallback, int_fast32_t limit)
{
    if(!specified)
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

static nodelist *make_result_nodelist(document_model *model)
{
    node *document = model_get_document(model, 0);
    nodelist *list = make_nodelist();
    ENSURE_NONNULL_ELSE_NULL(errno, list);
    nodelist_add(list, document);

    return list;
}


