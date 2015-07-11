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

#include "jsonpath/model.h"

static inline void step_destructor(void *each);


static inline void jsonpath_free(JsonPath *path)
{
    if(NULL == path)
    {
        return;
    }
    vector_destroy(path->steps, step_destructor);
    free(path->steps);
    free(path);
}

void path_free(MaybeJsonPath result)
{
    switch(result.tag)
    {
        case ERROR:
            free(result.error.message);
            break;
        case JSONPATH:
            jsonpath_free(result.path);
            break;
    }
}

static void predicate_free(Predicate *value)
{
    if(NULL == value)
    {
        return;
    }
    if(JOIN == predicate_kind(value))
    {
        jsonpath_free(value->join.left);
        jsonpath_free(value->join.right);
    }

    free(value);
}

static inline void step_free(Step *value)
{
    if(NULL == value)
    {
        return;
    }
    if(NAME_TEST == value->test.kind)
    {
        if(NULL != value->test.name.value)
        {
            free(value->test.name.value);
            value->test.name.value = NULL;
            value->test.name.length = 0;
        }
    }
    if(NULL != value->predicate)
    {
        predicate_free(value->predicate);
    }
    free(value);
}

static inline void step_destructor(void *each)
{
    step_free((Step *)each);
}
