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

#include "jsonpath/combinators.h"

combinator *jsonpath(void);
combinator *absolute_path(void);
combinator *root_step(void);
combinator *qualified_step(void);
combinator *recursive_step(void);
combinator *relative_step(void);
combinator *relative_path(void);
combinator *relative_head_step(void);
combinator *step(void);

combinator *transformer(void);
combinator *mapping(void);
combinator *key_value(void);
combinator *sequence(void);
combinator *value(void);

combinator *selector(void);
combinator *tag_selector(void);
combinator *anchor_selector(void);
combinator *type_selector(void);
combinator *type(void);

combinator *name_selector(void);
combinator *wildcard(void);
combinator *name(void);

combinator *predicate_expression(void);
combinator *predicate(void);
combinator *subscript(void);
combinator *slice(void);
combinator *join(void);

combinator *filter_expression(void);
combinator *or_expression(void);
combinator *and_expression(void);
combinator *comparison_expression(void);
combinator *comparison_op(void);
combinator *addititve_expression(void);
combinator *addititve_op(void);
combinator *multiplicative_expression(void);
combinator *multiplicative_op(void);
combinator *unary_expression(void);

combinator *boolean(void);


combinator *jsonpath(void)
{
    return alternation(absolute_path(), 
                       relative_path());
}

combinator *absolute_path(void)
{
    return concatenation(root_step(), repetition(qualified_step()));
}

combinator *root_step(void)
{
    return concatenation(literal("$"), option(predicate_expression()));
}

combinator *qualified_step(void)
{
    return alternation(recursive_step(), 
                       relative_step());
}

combinator *recursive_step(void)
{
    return concatenation(literal(".."), step());
}

combinator *relative_step(void)
{
    return concatenation(literal("."), step());
}

combinator *relative_path(void)
{
    return concatenation(alternation(relative_head_step(), step()), 
                         repetition(qualified_step()));
}

combinator *relative_head_step(void)
{
    return concatenation(literal("@"), option(predicate_expression()));
}

combinator *step(void)
{
    return alternation(transformer(), 
                       concatenation(selector(), option(predicate_expression())));
}

combinator *transformer(void)
{
    return concatenation(literal("="), value());
}

combinator *mapping(void)
{
    return concatenation(literal("{"),
                         option(concatenation(key_value(), 
                                              repetition(
                                                  concatenation(
                                                      literal(","), key_value())))),
                         literal("}"));
}

combinator *key_value(void)
{
    return concatenation(string(), literal(":"), value());
}

combinator *sequence(void)
{
    return concatenation(literal("["),
                         option(concatenation(value(), 
                                              repetition(
                                                  concatenation(
                                                      literal(","), value())))),
                         literal("]"));
}

combinator *value(void)
{
    return alternation(sequence(), 
                       mapping(), 
                       addititve_expression());
}

combinator *selector(void)
{
    return alternation(tag_selector(),
                       anchor_selector(),
                       type_selector(),
                       name_selector());
}

combinator *tag_selector(void)
{
    return concatenation(literal("!"), name());
}

combinator *anchor_selector(void)
{
    return concatenation(literal("&"), name());
}

combinator *type_selector(void)
{
    return concatenation(type(), literal("("), literal(")"));
}

combinator *type(void)
{
    return alternation(literal("object"),
                       literal("array"),
                       literal("string"),
                       literal("number"),
                       literal("integer"),
                       literal("decimal"),
                       literal("timestamp"),
                       literal("boolean"),
                       literal("null"));
}

combinator *name_selector(void)
{
    return alternation(wildcard(), name());
}

combinator *wildcard(void)
{
    return literal("*");
}

combinator *name(void)
{
    return alternation(quoted_string(), string());
}

combinator *predicate_expression(void)
{
    return concatenation(literal("["), predicate(), literal("]"));
}

combinator *predicate(void)
{
    return alternation(wildcard(),
                       subscript(),
                       slice(),
                       join(),
                       filter_expression());
}

combinator *subscript(void)
{
    return signed_integer();
}

combinator *slice(void)
{
    return concatenation(option(signed_integer()), literal(":"), option(signed_integer()), 
                         option(concatenation(literal(":"), option(non_zero_signed_integer()))));
}

combinator *join(void)
{
    return concatenation(addititve_expression(), literal(","), addititve_expression(), 
                         repetition(concatenation(literal(","), addititve_expression())));
}

combinator *filter_expression(void)
{
    return concatenation(literal("?("), or_expression(), literal(")"));
}

combinator *or_expression(void)
{
    return concatenation(and_expression(), 
                         option(concatenation(literal("or"), or_expression())));
}

combinator *and_expression(void)
{
    return concatenation(comparison_expression(),
                         option(concatenation(literal("and"), and_expression())));
}

combinator *comparison_expression(void)
{
    return concatenation(addititve_expression(),
                         option(concatenation(comparison_op(), comparison_expression())));
}

combinator *comparison_op(void)
{
    return alternation(literal(">"),
                       literal("<"),
                       literal(">="),
                       literal(">="),
                       literal("="),
                       literal("!="));
}

combinator *addititve_expression(void)
{
    return concatenation(multiplicative_expression(),
                         option(concatenation(addititve_op(), addititve_expression())));
}

combinator *addititve_op(void)
{
    return alternation(literal("+"), literal("-"));
}

combinator *multiplicative_expression(void)
{
    return concatenation(unary_expression(),
                         option(concatenation(multiplicative_op(), multiplicative_expression())));
}

combinator *multiplicative_op(void)
{
    return alternation(literal("*"), literal("/"), literal("%"));
}

combinator *unary_expression(void)
{
    return alternation(relative_path(),
                       number(),
                       string(),
                       boolean(),
                       literal("null"));
}

combinator *boolean(void)
{
    return alternation(literal("true"), literal("false"));
}
