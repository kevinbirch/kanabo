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


#include "vector.h"

#include "parser/syntax.h"


struct syntax_node_s
{
    uint_fast16_t type;

    Location  location;
    String   *value;
    Vector   *children;
};


SyntaxNode *make_syntax_node(uint_fast16_t type, String *value, Location location)
{
    SyntaxNode *self = calloc(1, sizeof(SyntaxNode));

    if(NULL != self)
    {
        self->type = type;
        self->location = location;
        self->value = value;
    }
    return self;
}

static void syntax_node_destructor(void *value)
{
    SyntaxNode *self = (SyntaxNode *)value;
    dispose_syntax_node(self);
}

void dispose_syntax_node(SyntaxNode *self)
{
    if(NULL == self)
    {
        return;
    }

    if(NULL != self->children)
    {
        vector_destroy(self->children, syntax_node_destructor);
    }
    if(NULL != self->value)
    {
        string_free(self->value);
    }
    free(self);
}

uint_fast16_t syntax_node_type(SyntaxNode *self)
{
    return self->type;
}

inline String *syntax_node_value(SyntaxNode *self)
{
    return self->value;
}

void syntax_node_add_child(SyntaxNode *self, SyntaxNode *child)
{
    if(NULL == self || NULL == child)
    {
        return;
    }

    if(NULL == self->children)
    {
        self->children = make_vector();
        if(NULL == self->children)
        {
            return;
        }
    }

    vector_add(self->children, child);
}

struct iteration_context_s
{
    SyntaxNodeVisitor visitor;
    void *parameter;
};

static bool visitor_adapter(void *each, void *parameter)
{
    SyntaxNode *node = (SyntaxNode *)each;
    struct iteration_context_s *context = (struct iteration_context_s *)parameter;
    syntax_node_visit_pre_order(node, context->visitor, context->parameter);

    return true;
}

void syntax_node_visit_pre_order(SyntaxNode *self, SyntaxNodeVisitor visitor, void *parameter)
{
    if(NULL == self || NULL == visitor)
    {
        return;
    }

    visitor(self, parameter);
    if(NULL != self->children)
    {
        struct iteration_context_s context = {visitor, parameter};
        vector_iterate(self->children, visitor_adapter, &context);
    }
}
