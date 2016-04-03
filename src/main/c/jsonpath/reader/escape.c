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


#include "jsonpath.h"
#include "jsonpath/escape.h"


static MaybeString build_unicode_escape(const char *value)
{
    MutableString *escape = make_mstring(6);
    if(NULL == escape)
    {
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    if(!mstring_append(&escape, "\\u"))
    {
        mstring_free(escape);
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    if(!mstring_append(&escape, value))
    {
        mstring_free(escape);
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }

    return just_string(escape);
}

static MaybeString build_escape_from_input(Input *input, size_t length)
{
    if(length > remaining(input))
    {
        consume_many(input, remaining(input));
        return nothing_string(ERR_PARSER_END_OF_INPUT);
    }
    MutableString *result = make_mstring(6);
    if(NULL == result)
    {
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    if(!mstring_append(&result, "\\u"))
    {
        mstring_free(result);
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    if(4 > length)
    {
        for(size_t i = 0; i < 4 - length; i++)
        {
            if(!mstring_append(&result, (uint8_t)'0'))
            {
                mstring_free(result);
                return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
            }
        }
    }
    if(!mstring_append_stream(&result, cursor(input), length))
    {
        mstring_free(result);
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    consume_many(input, length);
    return just_string(result);
}

MaybeString unescape_type(uint8_t type)
{
    MutableString *result = make_mstring_with_char(type);
    if(NULL == result)
    {
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    return just_string(result);
}

static MaybeString build_escape_from_type(uint8_t type)
{
    MutableString *result = make_mstring(2);
    if(NULL == result)
    {
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    if(!mstring_append(&result, (uint8_t)'\\'))
    {
        mstring_free(result);
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    if(!mstring_append(&result, type))
    {
        mstring_free(result);
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    return just_string(result);
}

MaybeString unescape(uint8_t type, Input *input)
{
    switch(type)
    {
        case '"':
        case '/':
        case ' ':
            // unescape as the literal value
            return unescape_type(type);
        case '\\':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
            // preserve these escape sequence
            return build_escape_from_type(type);
        case '_':
            return build_unicode_escape("00a0");
        case '0':
            return build_unicode_escape("0000");
        case 'a':
            return build_unicode_escape("0007");
        case 'e':
            return build_unicode_escape("001b");
        case 'v':
            return build_unicode_escape("000b");
        case 'L':
            return build_unicode_escape("2028");
        case 'N':
            return build_unicode_escape("0085");
        case 'P':
            return build_unicode_escape("2029");
        case 'x':
            // preserve this multi-character escape sequence
            return build_escape_from_input(input, 2);
        case 'u':
            // preserve this multi-character escape sequence
            return build_escape_from_input(input, 4);
        case 'U':
            // preserve this multi-character escape sequence
        {
            MaybeString upper = build_escape_from_input(input, 4);
            if(is_nothing(upper))
            {
                return upper;
            }
            MaybeString lower = build_escape_from_input(input, 4);
            if(is_nothing(lower))
            {
                free(value(upper));
                return lower;
            }
            mstring_append(&value(upper), value(lower));
            free(value(lower));
            return upper;
        }
        default:
            return nothing_string(ERR_ESCAPE_SEQUENCE);
    }
}
