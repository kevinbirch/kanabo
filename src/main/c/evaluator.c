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

struct predicate_parameter_block
{
    step *current_step;
    union
    {
        nodelist_to_one_function to_one_predicate;
        nodelist_to_many_function to_many_predicate;
    };
};

typedef struct predicate_parameter_block predicate_parameter_block;

static nodelist *evaluate_steps(document_model *model, jsonpath *path);
static bool evaluate_step(step *current, nodelist **list);
static bool evaluate_single_step(step *current, nodelist **list);

static inline bool evaluate_wildcard_test(nodelist **list);
static bool apply_wildcard_test(node *each, void *context, nodelist *target);

static inline bool evaluate_type_test(step *current, nodelist *list);
static node *apply_type_test(node *each, void *context);

static inline bool evaluate_name_test(step *current, nodelist **list);
static node *apply_name_test(node *object, void *context);

static bool evaluate_predicated_name_test(step *current, nodelist **list);
static node *apply_to_one_predicate(node *each, void *context);
static bool apply_to_many_predicate(node *each, void *context, nodelist *target);
static bool apply_wildcard_predicate(node *each, void *context, nodelist *target);
static node *apply_subscript_predicate(node *object, void *context);
static bool apply_slice_predicate(node *each, void *context, nodelist *target);

static bool add_values_to_nodelist_map_iterator(node *key, node *value, void *context);
static node *make_boolean_node(bool value);
static size_t normalize(bool specified, int_fast32_t actual, size_t fallback, size_t length);
static nodelist *make_result_nodelist(document_model *model);

nodelist *evaluate(document_model *model, jsonpath *path)
{
    PRECOND_NONNULL_ELSE_NULL(model, path, model_get_document_root(model, 0));
    PRECOND_ELSE_NULL(ABSOLUTE_PATH == path->kind, 0 < model_get_document_count(model))

    return evaluate_steps(model, path);
}

static nodelist *evaluate_steps(document_model *model, jsonpath *path)
{
    nodelist *list = make_result_nodelist(model);
    ENSURE_NONNULL_ELSE_NULL(errno, list);

    for(size_t i = 0; i < path_get_length(path); i++)
    {
        step *current = path_get_step(path, i);
        if(!evaluate_step(current, &list))
        {
            nodelist_free_nodes(list);
            nodelist_free(list);
            return NULL;
        }
    }

    return list;
}

static bool evaluate_step(step *current, nodelist **list)
{
    bool result = false;
    switch(step_get_kind(current))
    {
        case ROOT:
            result = nodelist_set(*list, document_get_root(nodelist_get(*list, 0)), 0);
            break;
        case SINGLE:
            result = evaluate_single_step(current, list);
            break;
        case RECURSIVE:
            // xxx - implement me!
            result = false;
            break;
    }
    return result;
}

static bool evaluate_single_step(step *current, nodelist **list)
{
    bool result = false;
    switch(step_get_test_kind(current))
    {
        case WILDCARD_TEST:
            result = evaluate_wildcard_test(list);
            break;
        case TYPE_TEST:
            result = evaluate_type_test(current, *list);
            break;
        case NAME_TEST:
            result = evaluate_name_test(current, list);
            break;
    }
    return result;
}

static inline bool evaluate_wildcard_test(nodelist **list)
{
    nodelist *result = nodelist_flatmap(*list, apply_wildcard_test, NULL);
    return NULL == result ? false : (nodelist_free(*list), *list = result, true);
}

static bool apply_wildcard_test(node *each, void *context, nodelist *target)
{
#pragma unused(context)
    bool result = false;
    switch(node_get_kind(each))
    {
        case MAPPING:
            result = iterate_mapping(each, add_values_to_nodelist_map_iterator, target);
            break;
        case SEQUENCE:
            result = iterate_sequence(each, add_to_nodelist_sequence_iterator, target);
            break;
        case SCALAR:
            result = nodelist_add(target, each);
            break;
        case DOCUMENT:
            errno = ERR_MISPLACED_DOCUMENT_NODE;
            result = false;
            break;
    }
    return result;
}

static inline bool evaluate_type_test(step *current, nodelist *list)
{
    return NULL != nodelist_map_overwrite(list, apply_type_test, current, list);
}

static node *apply_type_test(node *each, void *context)
{
    step *step_context = (step *)context;
    bool result = false;
    switch(type_test_step_get_type(step_context))
    {
        case OBJECT_TEST:
            result = MAPPING == node_get_kind(each);
            break;
        case ARRAY_TEST:
            result = SEQUENCE == node_get_kind(each);
            break;
        case STRING_TEST:
            result = SCALAR_STRING == scalar_get_kind(each);
            break;
        case NUMBER_TEST:
            result = SCALAR_NUMBER == scalar_get_kind(each);
            break;
        case BOOLEAN_TEST:
            result = SCALAR_BOOLEAN == scalar_get_kind(each);
            break;
        case NULL_TEST:
            result = SCALAR_NULL == scalar_get_kind(each);
            break;
    }
    return make_boolean_node(result);
}

static inline bool evaluate_name_test(step *current, nodelist **list)
{
    if(step_has_predicate(current))
    {
        return evaluate_predicated_name_test(current, list);
    }
    else
    {
        return NULL != nodelist_map_overwrite(*list, apply_name_test, current, *list);
    }
}

static node *apply_name_test(node *object, void *context)
{
    step *step_context = (step *)context;
    ENSURE_ELSE_NULL(ERR_NAME_IS_NOT_MAPPING, MAPPING == node_get_kind(object));
    node *value = mapping_get_value_scalar_key(object, name_test_step_get_name(step_context), name_test_step_get_length(step_context));
    ENSURE_NONNULL_ELSE_NULL(ERR_KEY_NOT_IN_MAPPING, value);
    return value;
}

static bool evaluate_predicated_name_test(step *current, nodelist **list)
{
    predicate_parameter_block block;
    block.current_step = current;
    nodelist *evaluated;
    bool result = false;

    switch(predicate_get_kind(step_get_predicate(current)))
    {
        case WILDCARD:
            block.to_many_predicate = apply_wildcard_predicate;
            evaluated = nodelist_flatmap(*list, apply_to_many_predicate, &block);
            result = NULL == evaluated ? false : (nodelist_free(*list), *list = evaluated, true);
            break;
        case SUBSCRIPT:
            block.to_one_predicate = apply_subscript_predicate;
            result = NULL != nodelist_map_overwrite(*list, apply_to_one_predicate, &block, *list);
            break;
        case SLICE:
            block.to_many_predicate = apply_slice_predicate;
            evaluated = nodelist_flatmap(*list, apply_to_many_predicate, &block);
            result = NULL == evaluated ? false : (nodelist_free(*list), *list = evaluated, true);
            break;
        case JOIN:
            // xxx - implement me!
            //return evaluate_join_predicate(current, list);
            result = false;
            break;
    }
    return result;
}

static node *apply_to_one_predicate(node *each, void *context)
{
    predicate_parameter_block *block = (predicate_parameter_block *)context;
    node *value = apply_name_test(each, block->current_step);
    ENSURE_NONNULL_ELSE_NULL(errno, value);
    return block->to_one_predicate(value, block->current_step);
}

static bool apply_to_many_predicate(node *each, void *context, nodelist *target)
{
    predicate_parameter_block *block = (predicate_parameter_block *)context;
    node *value = apply_name_test(each, block->current_step);
    ENSURE_NONNULL_ELSE_NULL(errno, value);
    return block->to_many_predicate(value, block->current_step, target);
}

static bool apply_wildcard_predicate(node *each, void *context, nodelist *target)
{
#pragma unused(context)
    bool result = false;
    switch(node_get_kind(each))
    {
        case SCALAR:
        case MAPPING:
            result = nodelist_add(target, each);
            break;
        case SEQUENCE:
            result = iterate_sequence(each, add_to_nodelist_sequence_iterator, target);
            break;
        case DOCUMENT:
            errno = ERR_MISPLACED_DOCUMENT_NODE;
            result = false;
            break;
    }
    return result;
}

static node *apply_subscript_predicate(node *each, void *context)
{
    step *step_context = (step *)context;
    ENSURE_ELSE_NULL(ERR_NAME_IS_NOT_SEQUENCE, SEQUENCE == node_get_kind(each));
    size_t index = subscript_predicate_get_index(step_get_predicate(step_context));
    return sequence_get(each, index);
}

static bool apply_slice_predicate(node *each, void *context, nodelist *target)
{
    ENSURE_ELSE_FALSE(ERR_NAME_IS_NOT_SEQUENCE, SEQUENCE == node_get_kind(each));

    predicate *predicate = step_get_predicate((step *)context);
    size_t from = normalize(slice_predicate_has_from(predicate), slice_predicate_get_from(predicate), 0, node_get_size(each));
    size_t to =   normalize(slice_predicate_has_to(predicate), slice_predicate_get_to(predicate), node_get_size(each), node_get_size(each));
    int_fast32_t step = slice_predicate_has_step(predicate) ? slice_predicate_get_step(predicate) : 1;
    
    for(size_t i = from; i > to; i = i + step)
    {
        errno = 0;
        node *selected = sequence_get(each, i);
        if(NULL == selected || 0 != errno || !nodelist_add(target, selected))
        {
            return false;
        }
    }
    return true;
}

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

static node *make_boolean_node(bool value)
{
    char *scalar = value ? strdup("true") : strdup("false");
    ENSURE_NONNULL_ELSE_NULL(errno, scalar);
    node *result = make_scalar_node((unsigned char *)scalar, strlen(scalar), SCALAR_BOOLEAN);
    ENSURE_NONNULL_ELSE_NULL(errno, result);
    
    return result;
}

static size_t normalize(bool specified, int_fast32_t actual, size_t fallback, size_t length)
{
    if(!specified)
    {
        return fallback;
    }
    int_fast32_t result = actual;
    if(0 > result)
    {
        result += length;
    }
    if(0 > result)
    {
        return 0;
    }
    if(length < result)
    {
        return length;
    }
    return (size_t)result;
}

static nodelist *make_result_nodelist(document_model *model)
{
    node *document = model_get_document(model, 0);
    ENSURE_NONNULL_ELSE_NULL(ERR_NO_DOCUMENT_IN_MODEL, document);
    nodelist *list = make_nodelist();
    ENSURE_NONNULL_ELSE_NULL(errno, list);
    nodelist_add(list, document);

    return list;
}


