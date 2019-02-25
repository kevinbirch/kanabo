#ifdef __linux__
#define _POSIX_C_SOURCE 200809L  // for fileno
#endif

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>

#include "test.h"
#include "log.h"

static void handle_signal(int sigval)
{
    void *stack[20];
    int depth;

    if(SIGSEGV == sigval || SIGABRT == sigval)
    {
        depth = backtrace(stack, 20);
        fputs("Backtrace follows (most recent first):\n", stderr);
        backtrace_symbols_fd(stack, depth, fileno(stderr));
        signal(sigval, SIG_DFL);
    }

    raise(sigval);
}

int main(int argc, char **argv)
{
    if(SIG_ERR == signal(SIGSEGV, handle_signal))
    {
        perror(argv[0]);
        exit(EXIT_FAILURE);
    }
    if(SIG_ERR == signal(SIGABRT, handle_signal))
    {
        perror(argv[0]);
        exit(EXIT_FAILURE);
    }
    enable_logging();
    set_log_level_from_env();

    SRunner *runner = srunner_create(master_suite());
    srunner_add_suite(runner, scanner_suite());
    srunner_add_suite(runner, parser_suite());
    srunner_add_suite(runner, model_suite());
    srunner_add_suite(runner, nodelist_suite());
    srunner_add_suite(runner, loader_suite());
    /* srunner_add_suite(runner, evaluator_suite()); */

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
