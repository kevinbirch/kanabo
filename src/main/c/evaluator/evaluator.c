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

static inline bool evaluate_test(const char * restrict name, evaluator_context *context, nodelist_map_function function);

static bool apply_wildcard_test(node *each, void *argument, nodelist *target);
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

#define component_name "evaluator"

#define evaluator_info(FORMAT, ...)  log_info(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_debug(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_trace(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(TRACE, component_name, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)

#define current_step(CONTEXT) path_get((CONTEXT)->path, (CONTEXT)->current_step)

evaluator_context *make_evaluator(const document_model *model, const jsonpath *path)
{
    evaluator_debug("creating evaluator context");
    evaluator_context *context = (evaluator_context *)calloc(1, sizeof(evaluator_context));
    if(NULL == context)
    {
        evaluator_debug("uh oh! out of memory, can't allocate the evaluator context");
        return NULL;
    }
    if(NULL == model)
    {
        evaluator_debug("model is null");
        errno = EINVAL;
        context->code = ERR_MODEL_IS_NULL;
        return context;
    }
    if(NULL == path)
    {
        evaluator_debug("path is null");
        errno = EINVAL;
        context->code = ERR_PATH_IS_NULL;
        return context;
    }
    if(NULL == model_get_document(model, 0))
    {
        evaluator_debug("document is null");
        errno = EINVAL;
        context->code = ERR_NO_DOCUMENT_IN_MODEL;
        return context;
    }
    if(NULL == model_get_document_root(model, 0))
    {
        evaluator_debug("document root is null");
        errno = EINVAL;
        context->code = ERR_NO_ROOT_IN_DOCUMENT;
        return context;
    }
    if(ABSOLUTE_PATH != path_kind(path))
    {
        evaluator_debug("path is not absolute");
        errno = EINVAL;
        context->code = ERR_PATH_IS_NOT_ABSOLUTE;
        return context;
    }
    if(0 == path_length(path))
    {
        evaluator_debug("path is empty");
        errno = EINVAL;
        context->code = ERR_PATH_IS_EMPTY;
        return context;
    }

    nodelist *list = make_nodelist();
    if(NULL == list)
    {
        evaluator_debug("uh oh! out of memory, can't allocate the result nodelist");
        context->code = ERR_EVALUATOR_OUT_OF_MEMORY;
        return context;
    }
    context->list = list;
    context->model = model;
    context->path = path;

    return context;
}

enum evaluator_status_code evaluator_status(const evaluator_context * restrict context)
{
    return context->code;
}

void evaluator_free(evaluator_context *context)
{
    evaluator_debug("destroying evaluator context");
    if(NULL == context)
    {
        return;
    }
    context->model = NULL;
    context->path = NULL;
    context->list = NULL;

    free(context);
}

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

static bool evaluate_step(step* each, void *context)
{
    evaluator_context *step_context = (evaluator_context *)context;
    evaluator_trace("step: %zd", step_context->current_step);

    bool result = false;
    switch(step_kind(each))
    {
        case ROOT:
            result = evaluate_root_step(step_context);
            break;
        case SINGLE:
            result = evaluate_single_step(step_context);
            break;
        case RECURSIVE:
            result = evaluate_recursive_step(step_context);
            break;
    }
    if(result)
    {
        step_context->current_step++;
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
    evaluator_trace("evaluating recursive step across %zd nodes", nodelist_length(context->list));

    // xxx - implement me!
    evaluator_trace("recurisve step: uh oh! not implemented yet, aborting...");
    context->code = ERR_UNSUPPORTED_PATH;
    return false;
}

static bool evaluate_single_step(evaluator_context *context)
{
    bool result = false;
    evaluator_trace("evaluating single step across %zd nodes", nodelist_length(context->list));
    switch(step_test_kind(current_step(context)))
    {
        case WILDCARD_TEST:
            result = evaluate_test("wildcard test", context, apply_wildcard_test);
            break;
        case TYPE_TEST:
            result = evaluate_test("type test", context, apply_type_test);
            break;
        case NAME_TEST:
            result = evaluate_test("name test", context, apply_name_test);
            break;
    }
    if(result && step_has_predicate(current_step(context)))
    {
        result = evaluate_test("predicate", context, apply_predicate);
    }

    return result;
}

static inline bool evaluate_test(const char * restrict name, evaluator_context *context, nodelist_map_function function)
{
    evaluator_trace("evaluating %s across %zd nodes", name, nodelist_length(context->list));
    nodelist *result = nodelist_map(context->list, function, context);
    evaluator_trace("%s: %s", name, NULL == result ? "failed" : "completed");
    evaluator_trace("%s: added %zd nodes", name, nodelist_length(result));
    return NULL == result ? false : (nodelist_free(context->list), context->list = result, true);
}

static bool apply_wildcard_test(node *each, void *argument, nodelist *target)
{
    switch(node_get_kind(each))
    {
        case MAPPING:
            evaluator_trace("wildcard test: adding %zd mapping values (%p)", node_get_size(each), each);
            return iterate_mapping(each, add_values_to_nodelist_map_iterator, &(meta_context){(evaluator_context *)argument, target});
        case SEQUENCE:
            evaluator_trace("wildcard test: adding %zd sequence (%p) items", node_get_size(each), each);
            return iterate_sequence(each, add_to_nodelist_sequence_iterator, target);
        case SCALAR:
            trace_string("wildcard test: adding scalar: '%s' (%p)", scalar_get_value(each), node_get_size(each), each);
            return nodelist_add(target, each);
        case DOCUMENT:
            evaluator_trace("wildcard test: uh-oh! found a document node somehow (%p), aborting...", each);
            ((evaluator_context *)argument)->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            return false;
    }
}

static bool apply_type_test(node *each, void *argument, nodelist *target)
{
    bool match = false;
    switch(type_test_step_kind(current_step((evaluator_context *)argument)))
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
        return nodelist_add(target, each);
    }
    else
    {
        evaluator_trace("type test: no match (actual: %d). dropping (%p)", SCALAR == node_get_kind(each) ? scalar_get_kind(each) : node_get_kind(each), each);
        return true;
    }
}

static bool apply_name_test(node *each, void *argument, nodelist *target)
{
    step *context_step = current_step((evaluator_context *)argument);
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
    return nodelist_add(target, value);
}

static bool apply_predicate(node *each, void *argument, nodelist *target)
{
    evaluator_context *context = (evaluator_context *)argument;
    switch(predicate_kind(step_predicate(current_step(context))))
    {
        case WILDCARD:
            evaluator_trace("evaluating wildcard predicate");
            return apply_wildcard_predicate(each, context, target);
        case SUBSCRIPT:
            evaluator_trace("evaluating subscript predicate");
            return apply_subscript_predicate(each, context, target);
        case SLICE:
            evaluator_trace("evaluating slice predicate");
            return apply_slice_predicate(each, context, target);
        case JOIN:
            evaluator_trace("evaluating join predicate");
            return apply_join_predicate(each, context, target);
    }
}

static bool apply_wildcard_predicate(node *value, evaluator_context *context, nodelist *target)
{
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
            evaluator_trace("wildcard predicate: uh-oh! found a document node somehow (%p), aborting...", value);
            context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            return false;
    }
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
    return nodelist_add(target, selected);
}

static bool apply_slice_predicate(node *value, evaluator_context *context, nodelist *target)
{
    if(SEQUENCE != node_get_kind(value))
    {
        evaluator_trace("slice predicate: node is not a sequence type, cannot use a slice on it (kind: %d), dropping (%p)", node_get_kind(value), value);
    }

    predicate *slice = step_predicate(current_step(context));
    int_fast32_t from = 0, to = 0, step = 0;
    normalize_interval(value, slice, &from, &to, &step);
    evaluator_trace("slice predicate: using normalized interval [%d:%d:%d]", from, to, step);

    for(int_fast32_t i = from; 0 > step ? i >= to : i < to; i += step)
    {
        node *selected = sequence_get(value, (size_t)i);
        if(NULL == selected || !nodelist_add(target, selected))
        {
            evaluator_trace("slice predicate: uh oh! aborting. index: %d, selected: %p", i, selected);
            context->code = ERR_EVALUATOR_OUT_OF_MEMORY;
            return false;
        }
        evaluator_trace("slice predicate: adding index: %d (%p)", i, selected);         
    }
    return true;
}

static bool apply_join_predicate(node *value, evaluator_context *context, nodelist *target)
{
#pragma unused(value, target)
    evaluator_trace("join predicate: evaluating axes (_, _)");

    // xxx - implement me!
    evaluator_trace("join predicate: uh-oh! not implemented yet, aborting...");
    context->code = ERR_UNSUPPORTED_PATH;
    return false;
}

/* 
 * Utility Functions
 * =================
 */

static bool add_values_to_nodelist_map_iterator(node *key, node *value, void *context)
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
            evaluator_trace("wildcard test: uh-oh! found a document node somehow (%p), aborting...", value);
            iterator_context->context->code = ERR_UNEXPECTED_DOCUMENT_NODE;
            result = false;
            break;
    }
    return result;
}

static void normalize_interval(node *value, predicate *slice, int_fast32_t *from, int_fast32_t *to, int_fast32_t *step)
{
    char *from_fmt = NULL, *to_fmt = NULL, *step_fmt = NULL;
    evaluator_trace("slice predicate: evaluating interval [%s:%s:%s] on sequence (%p) of %zd items",
                    slice_predicate_has_from(slice) ? (asprintf(&from_fmt, "%d", slice_predicate_from(slice)), from_fmt) : "_",
                    slice_predicate_has_to(slice) ? (asprintf(&to_fmt, "%d", slice_predicate_to(slice)), to_fmt) : "_",
                    slice_predicate_has_step(slice) ? (asprintf(&step_fmt, "%d", slice_predicate_step(slice)), step_fmt) : "_",
                    value, node_get_size(value));
    free(from_fmt); free(to_fmt); free(step_fmt);
    *step = slice_predicate_has_step(slice) ? slice_predicate_step(slice) : 1;
    *from = 0 > *step ? normalize_to(slice, value) - 1 : normalize_from(slice, value);
    *to   = 0 > *step ? normalize_from(slice, value) : normalize_to(slice, value);
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

