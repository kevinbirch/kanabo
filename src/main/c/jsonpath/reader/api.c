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


#include "conditions.h"

#include "jsonpath/ast.h"
#include "jsonpath/grammar.h"
#include "jsonpath/model.h"
#include "jsonpath/logging.h"


#define PRECOND_ELSE_NOTHING(CODE, ...)                                 \
    if(is_false(__VA_ARGS__, -1))                                       \
    {                                                                   \
        return (MaybeJsonPath){PATH_ERROR, .error.code=(CODE), .error.position=0, .error.message=parser_status_message((CODE), 0)}; \
    }

#define path_error(CODE, POSITION) (MaybeJsonPath){PATH_ERROR, .error.code=(CODE), .error.position=(POSITION), .error.message=parser_status_message((CODE), (POSITION))}
#define just_path(VALUE) (MaybeJsonPath){PATH_VALUE, .value=(VALUE)}


static inline JsonPath *build_path(SyntaxNode *ast)
{
    if(NULL == ast)
    {
        return NULL;
    }
    return NULL;
}

static inline MaybeJsonPath transform(MaybeSyntaxNode *ast, Input *input)
{
    if(is_nothing(*ast))
    {
        return path_error(ast->code, position(input));
    }
    return (MaybeJsonPath){PATH_VALUE, .value=build_path(ast->value)};

}

MaybeJsonPath parse(const uint8_t *expression, size_t length)
{
    PRECOND_ELSE_NOTHING(ERR_NULL_EXPRESSION, NULL != expression);
    PRECOND_ELSE_NOTHING(ERR_ZERO_LENGTH, 0 != length);

    debug_string("parsing expression: '%s'", expression, length);

    Input input = make_input(expression, length);

    Parser *parser = jsonpath();
    MaybeSyntaxNode ast = just_ast(make_syntax_node(CST_ROOT, NULL, location_from_input(input)));
    MaybeSyntaxNode result = bind(parser, ast, &input);
    if(has_more(&input))
    {
        return path_error(ERR_UNEXPECTED_VALUE, position(&input));
    }
    // xxx - free parser!
    //parser_free(parser);

    return transform(&result, &input);
