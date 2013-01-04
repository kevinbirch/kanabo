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

bool evaluate_one_step(step *step, node *context, nodelist *list);
bool evaluate_type_test(step *step, node *context);
node *make_boolean_node(bool value);
void set_result(nodelist *list, node *value);


nodelist *evaluate(document_model *model, jsonpath *path)
{
    if(NULL == model || NULL == path || RELATIVE_PATH == path->kind || 0 == model_get_document_count(model) || NULL == model_get_document_root(model, 0))
    {
        errno = EINVAL;
        return NULL;
    }

    node *current = model_get_document(model, 0);
    if(NULL == current)
    {
        errno = EINVAL;
        return NULL;
    }
    nodelist *result = make_nodelist();
    if(NULL == result)
    {
        return NULL;
    }

    for(size_t i = 0; i < path_get_length(path); i++)
    {
        step *step = path_get_step(path, i);
        switch(step_get_kind(step))
        {
            case ROOT:
                current = document_get_root(current);
                nodelist_add(result, current);
                break;
            case SINGLE:
                if(!evaluate_one_step(step, current, result))
                {
                    nodelist_free(result);
                    return NULL;
                }
                // xxx - fix me
                current = nodelist_get(result, 0);
                break;
            case RECURSIVE:
                break;
        }
    }

    return result;
}

bool evaluate_one_step(step *step, node *context, nodelist *list)
{
    if(NAME_TEST == step_get_test_kind(step))
    {
        if(MAPPING != node_get_kind(context))
        {
            errno = EINVAL;
            return false;
        }
        if(0 == step_get_predicate_count(step))
        {
            node *result = mapping_get_value_scalar_key(context, name_test_step_get_name(step), name_test_step_get_length(step));
            if(NULL == result)
            {
                errno = EINVAL;
                return false;
            }
            set_result(list, result);
        }
        /* else */
        /* { */
        /*     evaluate_predicates(step, target, list); */
        /* } */
    }
    else
    {
        bool match = evaluate_type_test(step, context);
        node *result = make_boolean_node(match);
        if(NULL == result)
        {
            return false;
        }
        set_result(list, result);
    }

    return true;
}

bool evaluate_type_test(step *step, node *context)
{
    bool result = false;
    
    switch(type_test_step_get_type(step))
    {
        case OBJECT_TEST:
            result = MAPPING == node_get_kind(context);
            break;
        case ARRAY_TEST:
            result = SEQUENCE == node_get_kind(context);
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


