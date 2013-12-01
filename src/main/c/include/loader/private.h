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

#pragma once

#include <errno.h>
#include <regex.h>

#include "log.h"
#include "hashtable.h"

struct loader_context
{
    yaml_parser_t     *parser;
    loader_status_code code;
    enum loader_duplicate_key_strategy strategy;
    
    document_model    *model;

    node              *target;
    struct 
    {
        uint8_t *value;
        size_t   length;
    } key_holder;

    Hashtable *anchors;
    
    regex_t           *decimal_regex;
    regex_t           *integer_regex;
    regex_t           *timestamp_regex;
};

document_model *build_model(loader_context *context);
loader_status_code interpret_yaml_error(yaml_parser_t *parser);

#define component_name "loader"

#define loader_info(FORMAT, ...)  log_info(component_name, FORMAT, ##__VA_ARGS__)
#define loader_error(FORMAT, ...)  log_error(component_name, FORMAT, ##__VA_ARGS__)
#define loader_debug(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define loader_trace(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(LVL_TRACE, component_name, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)

