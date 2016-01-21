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

#include "jsonpath/maybe_ast.h"
#include "jsonpath/grammar.h"
#include "jsonpath/model.h"
#include "jsonpath/logging.h"

#undef nothing
#define nothing(CODE, CURSOR, MSG) (MaybeJsonPath){ERROR, .error.code=(CODE), .error.position=(CURSOR), .error.message=(MSG)}
#undef just
#define just(PATH) (MaybeJsonPath){JSONPATH, .path=(PATH)}

#define PRECOND_ELSE_NOTHING(ERR_CODE, ...)                       \
    if(is_false(__VA_ARGS__, -1))                                 \
    {                                                             \
        char *msg = parser_status_message((ERR_CODE), 0, NULL);   \
        return (MaybeJsonPath){.tag=ERROR, .error.code=(ERR_CODE), .error.message=msg}; \
    }


static inline JsonPath *build_path(Ast *ast)
{
    if(NULL == ast)
    {
        return NULL;
    }
    return NULL;
}

static inline MaybeJsonPath resolve_ast(MaybeAst *ast, Input *input)
{
    if(AST_ERROR == ast->tag)
    {
        char *msg = parser_status_message(ast->error.code,
                                          ast->error.argument,
                                          input);
        return nothing(ast->error.code, position(input), msg);
        
    }
    return (MaybeJsonPath){JSONPATH, .path=build_path(ast->value)};

}

static inline MaybeAst execute(Parser *parser, Input *input)
{
    MaybeAst ast = (MaybeAst){AST_VALUE, .value=make_ast_root_node()};
    return bind(parser, ast, input);
}

MaybeJsonPath parse(const uint8_t *expression, size_t length)
{
    PRECOND_ELSE_NOTHING(ERR_NULL_EXPRESSION, NULL != expression);
    PRECOND_ELSE_NOTHING(ERR_ZERO_LENGTH, 0 != length);

    debug_string("parsing expression: '%s'", expression, length);

    Input input = make_input(expression, length);

    Parser *parser = jsonpath();
    MaybeAst ast = execute(parser, &input);
    parser_free(parser);

    return resolve_ast(&ast, &input);
}
