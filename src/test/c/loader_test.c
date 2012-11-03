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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "loader.h"

static const char *yaml =
    "one:\n"
    "  - foo1"
    "  - bar1\n"
    "\n"
    "two: foo2\n"
    "\n"
    "three: foo3\n"
    "\n"
    "four:\n"
    "  - foo4\n"
    "  - bar4\n"
    "\n"
    "five: foo5\n";

void test_loader(FILE *input);

void test_loader(FILE *input)
{
    document_model model;
    int result = load_file(input, &model);
    
    assert(0 == result);
    assert(1 == model.size);

    node *document = model.documents[0];
    
    assert(DOCUMENT == document->tag.kind);

    node *root = document->content.document.root;
    
    assert(MAPPING == root->tag.kind);
    assert(5 == root->content.size);
}

int main()
{
    // xxx - use fmemopen impl here instead of temp file
    FILE *data = tmpfile();
    size_t written = fwrite(yaml, sizeof(char), strlen(yaml), data);
    if(written != strlen(yaml))
    {
        fprintf(stderr, "unable to write temp file\n");
        return 1;
    }
    
    rewind(data);
    test_loader(data);
    
    return 0;
}
