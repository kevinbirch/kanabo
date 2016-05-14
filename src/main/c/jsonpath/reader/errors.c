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


#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#endif

#ifdef __APPLE__
#define _DARWIN_SOURCE
#endif

#include <stdio.h>
#include <string.h>

#include "conditions.h"
#include "jsonpath.h"
#include "jsonpath/messages.h"


typedef char *(*MessageHandler)(const char *message, SourceLocation location);

struct handler_s
{
    MessageHandler formatter;
    const char * const message;
};

static char *just_dup_it(const char *message, SourceLocation location __attribute__((unused)))
{
    return strdup(message);
}

static char *format_it(const char *format, SourceLocation srcloc)
{
    char *message = NULL;
    int result = 0;
    result = asprintf(&message, format, srcloc.location.offset + 1);
    return -1 == result ? NULL : message;
}

static struct handler_s HANDLERS[] =
{
    {just_dup_it, "Success."},
    {just_dup_it, "Unable to allocate memory."},
    {just_dup_it, "No input data to parse."},
    {just_dup_it, "Premature end of input."},
    {format_it, "At position %zd: unexpected character."},
    {format_it, "At position %zd: expected a name character."},
    {format_it, "At position %zd: invalid control character."},
    {format_it, "At position %zd: unsupported escape sequence."},
    {format_it, "At position %zd: empty predicate."},
    {format_it, "At position %zd: missing closing predicate delimiter `]' before end of step."},
    {format_it, "At position %zd: unsupported predicate found."},
    {format_it, "At position %zd: extra characters after valid predicate definition."},
    {format_it, "At position %zd: expected a node type test."},
    {format_it, "At position %zd: expected an integer."},
    {format_it, "At position %zd: invalid number."},
    {format_it, "At position %zd: slice step value must be non-zero."}
};


char *status_message(uint_fast16_t code, SourceLocation srcloc)
{
    if(ERR_CODE_MAX < code)
    {
        return NULL;
    }
    struct handler_s *handler = &HANDLERS[code];
    return handler->formatter(handler->message, srcloc);
}
