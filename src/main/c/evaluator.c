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
#include "preconditions.h"

nodelist *evaluate_steps(document_model *model, jsonpath *path);
bool evaluate_one_step(step *step, nodelist *list);
bool evaluate_single_step(step *step, nodelist *list);
bool evaluate_wildcard_test(nodelist *list);
bool apply_wildcard_test(node *each, void *context);
bool add_values_to_nodelist(node *key, node *value, void *context);
bool add_elements_to_nodelist(node *each, void *context);
bool evaluate_name_test(step *step, nodelist *list);
bool evaluate_simple_name_test(step *step, nodelist *list);
bool evaluate_predicated_name_test(step *step, nodelist *list);
bool evaluate_wildcard_predicate(step *step, nodelist *list);
bool apply_wildcard_predicate(node *each, void *context);
bool evaluate_subscript_predicate(step *step, nodelist *list);
bool evaluate_type_test(step *step, nodelist *list);
node *apply_type_test(node *each, void *context);

nodelist *make_result_nodelist(document_model *model);
node *make_boolean_node(bool value);
nodelist *nodelist_clone(nodelist *list);

nodelist *evaluate(document_model *model, jsonpath *path)
{
    ENSURE_NONNULL_ELSE_NULL(model, path, model_get_document_root(model, 0));
    ENSURE_COND_ELSE_NULL(ABSOLUTE_PATH == path->kind, 0 < model_get_document_count(model))

    return evaluate_steps(model, path);
}

nodelist *evaluate_steps(document_model *model, jsonpath *path)
{
    nodelist *list = make_result_nodelist(model);
    if(NULL == list)
    {
        return NULL;
    }

    for(size_t i = 0; i < path_get_length(path); i++)
    {
        step *step = path_get_step(path, i);
        if(!evaluate_one_step(step, list))
        {
            nodelist_free_nodes(list);
            nodelist_free(list);
            return NULL;
        }
    }

    return list;
}

bool evaluate_one_step(step *step, nodelist *list)
{
    switch(step_get_kind(step))
    {
        case ROOT:
            return nodelist_set(list, document_get_root(nodelist_get(list, 0)), 0);
        case SINGLE:
            return evaluate_single_step(step, list);
        case RECURSIVE:
            return false;
    }
}

bool evaluate_single_step(step *step, nodelist *list)
{
    switch(step_get_test_kind(step))
    {
        case WILDCARD_TEST:
            return evaluate_wildcard_test(list);
        case NAME_TEST:
            return evaluate_name_test(step, list);
        case TYPE_TEST:
            return evaluate_type_test(step, list);
    }
}

bool evaluate_wildcard_test(nodelist *list)
{
    if(1 == nodelist_length(list))
    {
        node *context = nodelist_get(list, 0);
        nodelist_clear(list);

        return apply_wildcard_test(context, list);
    }
    else
    {
        nodelist *clone = nodelist_clone(list);
        if(NULL == clone)
        {
            return false;
        }
        nodelist_clear(list);
        bool result = nodelist_iterate(clone, apply_wildcard_test, list);
        nodelist_free(clone);
        return result;
    }
}

bool apply_wildcard_test(node *each, void *context)
{
    nodelist *list = (nodelist *)context;
    switch(node_get_kind(each))
    {
        case MAPPING:
            return iterate_mapping(each, add_values_to_nodelist, list);
        case SEQUENCE:
            return iterate_sequence(each, add_elements_to_nodelist, list);
        case SCALAR:
        case DOCUMENT:
            // xxx - signal error
            errno = EINVAL;
            return false;
    }
}

bool add_values_to_nodelist(node *key, node *value, void *context)
{
#pragma unused(key)
    nodelist *list = (nodelist *)context;
    switch(node_get_kind(value))
    {
        case SCALAR:
        case MAPPING:
            return nodelist_add(list, value);
        case SEQUENCE:
            return iterate_sequence(value, add_elements_to_nodelist, context);
        case DOCUMENT:
            // xxx - signal error
            errno = EINVAL;
            return false;
    }
}

bool add_elements_to_nodelist(node *each, void *context)
{
    nodelist *list = (nodelist *)context;
    return nodelist_add(list, each);
}

bool evaluate_name_test(step *step, nodelist *list)
{
    if(step_has_predicate(step))
    {
        return evaluate_predicated_name_test(step, list);
    }
    else
    {
        return evaluate_simple_name_test(step, list);
    }
}

bool evaluate_simple_name_test(step *step, nodelist *list)
{
    bool result = true;
    for(size_t i = 0; i < nodelist_length(list) && true == result; i++)
    {
        node *each = nodelist_get(list, i);
        if(MAPPING != node_get_kind(each))
        {
            // xxx - add error states
            errno = EINVAL;
            return false;
        }
        node *value = mapping_get_value_scalar_key(each, name_test_step_get_name(step), name_test_step_get_length(step));
        if(NULL == value)
        {
            errno = EINVAL;
            return false;
        }
        result &= nodelist_set(list, value, i);
    }

    return result;
}

bool evaluate_predicated_name_test(step *step, nodelist *list)
{
    predicate *predicate = step_get_predicate(step);
    switch(predicate_get_kind(predicate))
    {
        case WILDCARD:
            return evaluate_wildcard_predicate(step, list);
        case SUBSCRIPT:
            return evaluate_subscript_predicate(step, list);
            return false;
        case SLICE:
            //return evaluate_slice_predicate(step, list);
            return false;
        case JOIN:
            //return evaluate_join_predicate(step, list);
            return false;
    }
}

bool evaluate_wildcard_predicate(step *step, nodelist *list)
{
    nodelist *clone = nodelist_clone(list);
    if(NULL == clone)
    {
        return false;
    }
    nodelist_clear(list);
    bool result = true;
    for(size_t i = 0; i < nodelist_length(clone) && true == result; i++)
    {
        node *each = nodelist_get(clone, i);
        if(MAPPING != node_get_kind(each))
        {
            // xxx - add error states
            errno = EINVAL;
            return false;
        }
        node *value = mapping_get_value_scalar_key(each, name_test_step_get_name(step), name_test_step_get_length(step));
        if(NULL == value)
        {
            errno = EINVAL;
            return false;
        }
        result &= apply_wildcard_predicate(value, list);
    }
    nodelist_free(clone);
    return result;
}

bool apply_wildcard_predicate(node *each, void *context)
{
    nodelist *list = (nodelist *)context;
    switch(node_get_kind(each))
    {
        case SCALAR:
        case MAPPING:
            return nodelist_add(list, each);
        case SEQUENCE:
            return iterate_sequence(each, add_elements_to_nodelist, list);
        case DOCUMENT:
            // xxx - signal error
            errno = EINVAL;
            return false;
    }
}

bool evaluate_subscript_predicate(step *step, nodelist *list)
{
    bool result = true;
    for(size_t i = 0; i < nodelist_length(list) && true == result; i++)
    {
        node *each = nodelist_get(list, i);
        if(MAPPING != node_get_kind(each))
        {
            // xxx - add error states
            errno = EINVAL;
            return false;
        }
        node *sequence = mapping_get_value_scalar_key(each, name_test_step_get_name(step), name_test_step_get_length(step));
        if(NULL == sequence || SEQUENCE != node_get_kind(sequence))
        {
            errno = EINVAL;
            return false;
        }
        size_t index = subscript_predicate_get_index(step_get_predicate(step));
        node *value = sequence_get(sequence, index);
        result &= nodelist_set(list, value, i);
    }

    return result;
}

bool evaluate_type_test(step *step, nodelist *list)
{
    nodelist *result = nodelist_map_overwrite(list, apply_type_test, step, list);    
    if(NULL == result)
    {
        return false;
    }
    return true;
}

node *apply_type_test(node *each, void *context)
{
    step *step_context = (step *)context;
    bool result;
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

node *make_boolean_node(bool value)
{
    char *scalar = value ? strdup("true") : strdup("false");
    
    node *result = make_scalar_node((unsigned char *)scalar, strlen(scalar), SCALAR_BOOLEAN);
    if(NULL == result)
    {
        return NULL;
    }
    
    return result;
}

nodelist *make_result_nodelist(document_model *model)
{
    node *document = model_get_document(model, 0);
    if(NULL == document)
    {
        // xxx - add error states
        errno = EINVAL;
        return NULL;
    }
    nodelist *list = make_nodelist();
    if(NULL == list)
    {
        return NULL;
    }
    nodelist_add(list, document);

    return list;
}

nodelist *nodelist_clone(nodelist *list)
{
    nodelist *clone = make_nodelist_with_capacity(nodelist_length(list));
    if(NULL == clone)
    {
        return NULL;
    }
    for(size_t i = 0; i < nodelist_length(list); i++)
    {
        nodelist_add(clone, nodelist_get(list, i));
    }

    return clone;
}


