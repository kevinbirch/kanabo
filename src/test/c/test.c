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

#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "test.h"
#include "log.h"

static void init_logging(void);

int main(int argc, char **argv)
{
    init_logging();

    SRunner *runner = srunner_create(master_suite());
    srunner_add_suite(runner, loader_suite());
    srunner_add_suite(runner, jsonpath_suite());
    srunner_add_suite(runner, model_suite());
    srunner_add_suite(runner, nodelist_suite());
    srunner_add_suite(runner, evaluator_suite());

    switch(argc)
    {
        case 1:
            srunner_run_all(runner, CK_NORMAL);
            break;
        case 2:
            srunner_run(runner, argv[1], NULL, CK_NORMAL);
            break;
        case 3:
            srunner_run(runner, argv[1], argv[2], CK_NORMAL);
            break;
    }

    int failures = srunner_ntests_failed(runner);
    srunner_free(runner);
    
    return 0 == failures ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void init_logging(void)
{
    enable_logging();

    char *level = getenv("KANABO_TEST_LOG_LEVEL");
    if(NULL == level)
    {
        set_log_level(INFO);
    }
    else if(0 == memcmp("ERROR", level, 5))
    {
        set_log_level(ERROR);
    }
    else if(0 == memcmp("WARNING", level, 7))
    {
        set_log_level(WARNING);
    }
    else if(0 == memcmp("INFO", level, 4))
    {
        set_log_level(INFO);
    }
    else if(0 == memcmp("DEBUG", level, 5))
    {
        set_log_level(DEBUG);
    }
    else if(0 == memcmp("TRACE", level, 5))
    {
        set_log_level(TRACE);
    }
    else
    {
        set_log_level(INFO);
    }

}
