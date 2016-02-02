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

#include "jsonpath/grammar.h"
#include "jsonpath/filters.h"


static Parser *absolute_path(void);
static Parser *root_step(void);
static Parser *qualified_step(void);
static Parser *recursive_step(void);
static Parser *relative_step(void);
static Parser *relative_path(void);
static Parser *relative_head_step(void);
static Parser *step(void);

/*
static Parser *transformer(void);
static Parser *object(void);
static Parser *key_value(void);
static Parser *array(void);
static Parser *value(void);
*/

static Parser *selector(void);
static Parser *tag_selector(void);
static Parser *anchor_selector(void);
static Parser *type_selector(void);
static Parser *type(void);

static Parser *name_selector(void);
static Parser *wildcard(void);
static Parser *name(void);

static Parser *predicate_expression(void);
static Parser *predicate(void);
static Parser *subscript(void);
static Parser *slice(void);
/*
static Parser *join(void);

static Parser *filter_expression(void);
static Parser *or_expression(void);
static Parser *and_expression(void);
static Parser *comparison_expression(void);
static Parser *comparison_op(void);
static Parser *addititve_expression(void);
static Parser *addititve_op(void);
static Parser *multiplicative_expression(void);
static Parser *multiplicative_op(void);
static Parser *unary_expression(void);
static Parser *scalar(void);

static Parser *boolean(void);
*/

/**
 * jsonpath
 *   = absolute path
 *   | relative path
 *   ;
 */
Parser *jsonpath(void)
{
    return rule(
        choice(
            absolute_path(),
            relative_path()));
}

// xxx - qualified_path parser?

/**
 * absolute path
 *   = root step, { qualified step }
 *   ;
 */
static Parser *absolute_path(void)
{
    return rule(
        sequence(
            root_step(),
            repetition(qualified_step())));
}

/**
 * root step
 *   = "$", [ predicate expression ]
 *   ;
 */
static Parser *root_step(void)
{
    return rule(
        sequence(
            literal("$"),
            option(predicate_expression())));
}

/**
 * qualified step
 *   = recursive step
 *   | relative step
 *   ;
 */
static Parser *qualified_step(void)
{
    return rule(
        choice(
            recursive_step(),
            relative_step()));
}

/**
 * recurisive step
 *   = "..", step
 *   ;
 */
static Parser *recursive_step(void)
{
    return rule(
        sequence(
            literal(".."),
            step()));
}

/**
 * relative step
 *   = ".", step
 *   ;
 */
static Parser *relative_step(void)
{
    return rule(
        sequence(
            literal("."),
            step()));
}

/**
 * relative path
 *   = ( relative head step | step ), { qualified step }
 *   ;
 */
static Parser *relative_path(void)
{
    return rule(
        sequence(
            choice(
                relative_head_step(),
                step()),
            repetition(qualified_step())));
}

/**
 * relative head step
 *   = "@" , [ predicate expression ]
 *   ;
 */
static Parser *relative_head_step(void)
{
    return rule(
        sequence(
            literal("@"),
            option(predicate_expression())));
}

/**
 * step
 *   = transformer
 *   | ( selector, [ predicate expression ] )
 *   ;
 */
static Parser *step(void)
{
    return rule(
        sequence(
            selector(),
            option(predicate_expression())));
}

/**
 * transformer
 *   = "=", value
 *   ;
 */
/*
static Parser *transformer(void)
{
    return rule(
        sequence(
            literal("="),
            value()));
}
*/

/**
 * object
 *   = "{",  [ key value, { ",", key value } ], "}"
 *   ;
 */
/*
static Parser *object(void)
{
    return rule(
        sequence(
            literal("{"),
            option(sequence(
                       key_value(),
                       repetition(sequence(
                                      literal(","),
                                      key_value())))),
            literal("}")));
}
*/

/**
 * key value
 *   = string, ":", value
 *   ;
 */
/*
static Parser *key_value(void)
{
    return rule(
        sequence(
            string(),
            literal(":"),
            value()));
}
*/

/**
 * array
 *   = "[", [ value, { ",", value } ], "]"
 *   ;
 */
/*
static Parser *array(void)
{
    return rule(
        sequence(
            literal("["),
            option(sequence(
                       value(),
                       repetition(sequence(
                                      literal(","),
                                      value())))),
            literal("]")));
}
*/

/**
 * value
 *   = array
 *   | object
 *   | additive expr
 *   ;
 */
/*
static Parser *value(void)
{
    // xxx - need a latch here to return a reference when tripped
    return rule(
        choice(
            scalar(),
            array(),
            object()));
    // xxx - reset latch here
}
*/

/**
 * selector
 *   = tag selector
 *   | anchor selector
 *   | type selector
 *   | name selector
 *   ;
 */
static Parser *selector(void)
{
    return rule(
        choice(
            tag_selector(),
            anchor_selector(),
            type_selector(),
            name_selector()));
}

/**
 * tag selector
 *   = "!", name
 *   ;
 */
static Parser *tag_selector(void)
{
    return rule(
        sequence(
            literal("!"),
            name()));
}

/**
 * anchor selector
 *   = "&", name
 *   ;
 */
static Parser *anchor_selector(void)
{
    return rule(
        sequence(
            literal("&"),
            name()));
}

/**
 * type selector
 *   = type, "(", ")"
 *   ;
 */
static Parser *type_selector(void)
{
    return rule(
        sequence(
            type(),
            literal("("),
            literal(")")));
}

/**
 * type
 *   = "object"
 *   | "array"
 *   | "string"
 *   | "number"
 *   | "integer"
 *   | "decimal"
 *   | "timestamp"
 *   | "boolean"
 *   | "null"
 *   ;
 */
static Parser *type(void)
{
    return rule(
        choice(
            literal("object"),
            literal("array"),
            literal("string"),
            literal("number"),
            literal("integer"),
            literal("decimal"),
            literal("timestamp"),
            literal("boolean"),
            literal("null")));
}

/**
 * name selector
 *   = wildcard
 *   | name
 *   ;
 */
static Parser *name_selector(void)
{
    return rule(
        choice(
            wildcard(),
            name()));
}

/**
 * wildcard
 *   = "*"
 *   ;
 */
static Parser *wildcard(void)
{
    return rule(
        literal("*"));
}

/**
 * name
 *   = "'", quoted name character, { quoted name character }, "'"
 *   | name character, { name character }
 *   ;
 *
 * quoted name character
 *     = ? any unicode character except ' or control characters ?
 *     | "\'"
 *     | escape
 *     ;
 *
 * name character
 *     = ? any unicode character except . or [ or control characters ?
 *     | "\."
 *     | "\["
 *     | escape
 *     ;
 */
static Parser *name(void)
{
    return rule(
        choice(
            sequence(
                literal("'"),
                term(quoted_name_filter, "'"),
                literal("'")),
            term(name_filter, ".[")));
}

/**
 * predicate expression
 *   = "[", predicate, "]"
 *   | filter expression
 *   ;
 */
static Parser *predicate_expression(void)
{
    return rule(
        sequence(
            literal("["),
            predicate(),
            literal("]")));
}

/**
 * predicate
 *   = wildcard
 *   | subscript
 *   | slice
 *   | join
 *   ;
 */
static Parser *predicate(void)
{
    return rule(
        choice(
            wildcard(),
            subscript(),
            slice()));
}

/**
 * subscript
 *    = signed integer
 *    ;
 */
static Parser *subscript(void)
{
    return rule(
        signed_integer());
}

/**
 * slice
 *   = [ signed integer ], ":", [ signed integer ], [ ":", [ non-zero signed integer ] ]
 *   ;
 */
static Parser *slice(void)
{
    return rule(
        sequence(
            option(
                signed_integer()),
            literal(":"),
            option(
                signed_integer()),
            option(
                sequence(
                    literal(":"),
                    option(non_zero_signed_integer())))));
}

/*
 * join
 *   = additive expr, ",", additive expr, { ",", additive expr }
 *   ;
 */
/*
static Parser *join(void)
{
    return rule(
        sequence(
            addititve_expression(),
            literal(","),
            addititve_expression(),
            repetition(
                sequence(
                    literal(","),
                    addititve_expression()))));
}

static Parser *filter_expression(void)
{
    return rule(
        sequence(
            literal("?("),
            or_expression(),
            literal(")")));
}

static Parser *or_expression(void)
{
    return rule(
        sequence(
            and_expression(),
            option(
                sequence(
                    literal("or"),
                    or_expression()))));
}

static Parser *and_expression(void)
{
    return rule(
        sequence(
            comparison_expression(),
            option(
                sequence(
                    literal("and"),
                    and_expression()))));
}

static Parser *comparison_expression(void)
{
    return rule(
        sequence(
            addititve_expression(),
            option(
                sequence(
                    comparison_op(),
                    comparison_expression()))));
}

static Parser *comparison_op(void)
{
    return rule(
        choice(
            literal(">"),
            literal("<"),
            literal(">="),
            literal(">="),
            literal("="),
            literal("!=")));
}

static Parser *addititve_expression(void)
{
    return rule(
        sequence(
            multiplicative_expression(),
            option(
                sequence(
                    addititve_op(),
                    addititve_expression()))));
}

static Parser *addititve_op(void)
{
    return rule(
        choice(
            literal("+"),
            literal("-")));
}

static Parser *multiplicative_expression(void)
{
    return rule(
        sequence(
            unary_expression(),
            option(
                sequence(
                    multiplicative_op(),
                    multiplicative_expression()))));
}

static Parser *multiplicative_op(void)
{
    return rule(
        choice(
            literal("*"),
            literal("/"),
            literal("%")));
}

static Parser *unary_expression(void)
{
    return rule(
        choice(
            relative_path(),
            scalar()));
}
*/

/**
 * scalar
 *   = number
 *   | string
 *   | boolean
 *   | "null"
 *   ;
 */
/*
static Parser *scalar(void)
{
    return rule(
        choice(
            number(),
            string(),
            boolean(),
            literal("null")));
}
*/

/**
 * boolean
 *   = "true"
 *   | "false"
 *   ;
 */
/*
static Parser *boolean(void)
{
    return rule(
        choice(
            literal("true"),
            literal("false")));
}
*/
