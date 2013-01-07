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

bool evaluate_single_step(step *step, nodelist *list);
bool evaluate_name_test(step *step, nodelist *list);
bool evaluate_simple_name_test(step *step, nodelist *list);
bool evaluate_type_test(step *step, nodelist *list);
bool evaluate_type_test_kind(step *step, nodelist *list);

node *make_boolean_node(bool value);
void set_result(nodelist *list, node *value);


nodelist *evaluate(document_model *model, jsonpath *path)
{
    if(NULL == model || NULL == path || RELATIVE_PATH == path->kind || 0 == model_get_document_count(model) || NULL == model_get_document_root(model, 0))
    {
        // xxx - add error states
        errno = EINVAL;
        return NULL;
    }

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

    for(size_t i = 0; i < path_get_length(path); i++)
    {
        step *step = path_get_step(path, i);
        switch(step_get_kind(step))
        {
            case ROOT:
                nodelist_set(list, document_get_root(nodelist_get(list, 0)), 0);
                break;
            case SINGLE:
                if(!evaluate_single_step(step, list))
                {
                    nodelist_free(list);
                    return NULL;
                }
                break;
            case RECURSIVE:
                break;
        }
    }

    return list;
}

bool evaluate_single_step(step *step, nodelist *list)
{
    bool result = false;
    switch(step_get_test_kind(step))
    {
        case NAME_TEST:
            result = evaluate_name_test(step, list);
            break;
        case WILDCARD_TEST:
            break;
        case TYPE_TEST:
            result = evaluate_type_test(step, list);
            break;
    }

    return result;
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
        nodelist_set(list, child, i);
    }

    return true;
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
    char *string = value ? strdup("true") : strdup("false");
    
    node *result = make_scalar_node((unsigned char *)string, strlen(string));
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


