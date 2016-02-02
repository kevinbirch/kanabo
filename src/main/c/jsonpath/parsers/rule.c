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


#include "jsonpath/parsers/base.h"


struct rule_parser_s
{
    Parser        base;
    const char   *name;
    Parser       *expression;
    ast_rewriter  rewriter;
};

typedef struct rule_parser_s RuleParser;


static void rule_free(Parser *value)
{
    free(value->repr);
    RuleParser *self = (RuleParser *)value;
    parser_free(self->expression);
}

static MaybeAst rule_delegate(Parser *parser, MaybeAst ast, Input *input)
{
    RuleParser *self = (RuleParser *)parser;
    ensure_more_input(input);

    return self->rewriter(bind(self->expression, ast, input));
}

MaybeAst default_rewriter(MaybeAst ast)
{
    return ast;
}

Parser *rule_parser(const char *name, Parser *expression, ast_rewriter rewriter)
{
    if(NULL == name)
    {
        return NULL;
    }
    // xxx - find rule parser in cache here
    // xxx - how to auto-initialize the rule cache and hold it elsewhere?
    if(NULL == expression)
    {
        return NULL;
    }
    RuleParser *self = (RuleParser *)calloc(1, sizeof(RuleParser));
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, RULE);
    self->base.vtable.delegate = rule_delegate;
    self->base.vtable.free = rule_free;
    asprintf(&self->base.repr, "rule %s", name);
    self->name = name;
    self->expression = expression;
    self->rewriter = rewriter;

    return (Parser *)self;
}



