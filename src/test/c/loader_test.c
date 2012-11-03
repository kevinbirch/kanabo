/*
 * 金棒 (kanabō)
 * Copyright (c) 2012 Kevin Birch <kmb@pobox.com>
 * 
 * 金棒 is a tool to bludgeon YAML and JSON files from the shell: the strong
 * made stronger.
 *
 * For more information, consult the README in the project root.
 *
 * Distributed under an [MIT-style][license] license.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * [license]: http://www.opensource.org/licenses/mit-license.php
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
    assert(DOCUMENT == model.documents[0]->tag.kind);
    assert(MAPPING == model.documents[0]->content.document.root->tag.kind);
    assert(5 == model.documents[0]->content.document.root->content.size);
    
}

int main()
{
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
