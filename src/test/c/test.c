#define _POSIX_C_SOURCE 200809L

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>

#include "test.h"
#include "log.h"

void handle_segv(int signal);

#ifdef CHECK_0_9_8
int main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    if(SIG_ERR == signal(SIGSEGV, handle_segv))
    {
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    enable_logging();
    set_log_level_from_env();

    SRunner *runner = srunner_create(master_suite());
    srunner_add_suite(runner, scanner_suite());
    srunner_add_suite(runner, jsonpath_suite());
    srunner_add_suite(runner, model_suite());
    srunner_add_suite(runner, nodelist_suite());
    srunner_add_suite(runner, loader_suite());
    srunner_add_suite(runner, evaluator_suite());

    switch(argc)
    {
        case 1:
            srunner_run_all(runner, CK_NORMAL);
            break;
#ifndef CHECK_0_9_8
        case 2:
            srunner_run(runner, argv[1], NULL, CK_NORMAL);
            break;
        case 3:
            srunner_run(runner, argv[1], argv[2], CK_NORMAL);
            break;
#endif
    }

    int failures = srunner_ntests_failed(runner);
    srunner_free(runner);
    
    return 0 == failures ? EXIT_SUCCESS : EXIT_FAILURE;
}

void handle_segv(int sigval)
{
    void *stack[20];
    int depth;
    
    if(SIGSEGV == sigval)
    {
        depth = backtrace(stack, 20);
        fprintf(stderr, "Backtrace follows (most recent first):\n");
        backtrace_symbols_fd(stack, depth, fileno(stderr));
        signal(SIGSEGV, SIG_DFL);
    }
    
    raise(sigval);
}
