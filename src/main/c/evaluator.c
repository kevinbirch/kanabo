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

nodelist *evaluate_steps(document_model *model, jsonpath *path);
bool evaluate_one_step(step *step, nodelist *list);
bool evaluate_single_step(step *step, nodelist *list);
bool evaluate_wildcard_test(nodelist *list);
bool evaluate_wildcard_test_once(node *context, nodelist *list);
void add_values_to_nodelist(node *key, node *value, void *context);
void add_elements_to_nodelist(node *each, void *context);
bool evaluate_name_test(step *step, nodelist *list);
bool evaluate_simple_name_test(step *step, nodelist *list);
bool evaluate_type_test(step *step, nodelist *list);
bool evaluate_type_test_kind(step *step, nodelist *list);

nodelist *make_result_nodelist(document_model *model);
node *make_boolean_node(bool value);
void set_result(nodelist *list, node *value);
nodelist *nodelist_clone(nodelist *list);

nodelist *evaluate(document_model *model, jsonpath *path)
{
    if(NULL == model || NULL == path || RELATIVE_PATH == path->kind || 0 == model_get_document_count(model) || NULL == model_get_document_root(model, 0))
    {
        // xxx - add error states
        errno = EINVAL;
        return NULL;
    }

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

        return evaluate_wildcard_test_once(context, list);
    }
    else
    {
        nodelist *clone = nodelist_clone(list);
        if(NULL == clone)
        {
            return false;
        }
        nodelist_clear(list);
        bool result = true;
        for(size_t i = 0; i < nodelist_length(clone); i++)
        {
            result &= evaluate_wildcard_test_once(nodelist_get(clone, i), list);
        }
        nodelist_free(clone);
        return result;
    }
}

bool evaluate_wildcard_test_once(node *context, nodelist *list)
{
    switch(node_get_kind(context))
    {
        case MAPPING:
            iterate_mapping(context, add_values_to_nodelist, list);
            return true;
        case SEQUENCE:
            iterate_sequence(context, add_elements_to_nodelist, list);
            return true;
        case SCALAR:
        case DOCUMENT:
            // xxx - signal error
            errno = EINVAL;
            return false;
    }
}

// xxx - switch the iterator return type to bool to propigate the evaluation status
void add_values_to_nodelist(node *key, node *value, void *context)
{
#pragma unused(key)
    nodelist *list = (nodelist *)context;
    switch(node_get_kind(value))
    {
        case SCALAR:
        case MAPPING:
            nodelist_add(list, value);
            break;
        case SEQUENCE:
            iterate_sequence(value, add_elements_to_nodelist, context);
            break;
        case DOCUMENT:
            // xxx - signal error
            errno = EINVAL;
    }
}

void add_elements_to_nodelist(node *each, void *context)
{
    nodelist *list = (nodelist *)context;
    nodelist_add(list, each);
}

bool evaluate_name_test(step *step, nodelist *list)
{
    if(0 == step_get_predicate_count(step))
    {
        return evaluate_simple_name_test(step, list);
    }
    else
    {
        return false;
        //evaluate_predicated_name_test(step, target, list);
    }
}

bool evaluate_simple_name_test(step *step, nodelist *list)
{
    bool result = true;
    for(size_t i = 0; i < nodelist_length(list); i++)
    {
        node *each = nodelist_get(list, i);
        if(MAPPING != node_get_kind(each))
        {
            // xxx - add error states
            errno = EINVAL;
            return false;
        }
        node *child = mapping_get_value_scalar_key(each, name_test_step_get_name(step), name_test_step_get_length(step));
        if(NULL == child)
        {
            errno = EINVAL;
            return false;
        }
        result &= nodelist_set(list, child, i);
    }

    return result;
}

bool evaluate_type_test(step *step, nodelist *list)
{
    bool match = evaluate_type_test_kind(step, list);
    node *result = make_boolean_node(match);
    if(NULL == result)
    {
        return false;
    }
    set_result(list, result);
    return true;
}

bool evaluate_type_test_kind(step *step, nodelist *list)
{
    bool result = true;
    enum node_kind expected_kind;

    switch(type_test_step_get_type(step))
    {
        case OBJECT_TEST:
            expected_kind = MAPPING;
            break;
        case ARRAY_TEST:
            expected_kind = SEQUENCE;
            break;
        case STRING_TEST:
            break;
        case NUMBER_TEST:
            break;
        case BOOLEAN_TEST:
            break;
        case NULL_TEST:
            break;
    }
    for(size_t i = 0; i < nodelist_length(list); i++)
    {
        result &= expected_kind == node_get_kind(nodelist_get(list, i));
    }

    return result;
}

node *make_boolean_node(bool value)
{
    char *scalar = value ? strdup("true") : strdup("false");
    
    node *result = make_scalar_node((unsigned char *)scalar, strlen(scalar));
    if(NULL == result)
    {
        return NULL;
    }
    
    return result;
}

void set_result(nodelist *list, node *value)
{
    if(!nodelist_is_empty(list))
    {
        nodelist_clear(list);
    }
    nodelist_add(list, value);
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


