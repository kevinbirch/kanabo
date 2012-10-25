
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "options.h"
#include "shell.h"

int dispatch(int command, struct settings *settings);

int main(const int argc, char * const *argv)
{
    if(NULL == argv || NULL == argv[0])
    {
        fprintf(stderr, "error: whoa! something is wrong, there are no program arguments.\n");
        exit(-1);
    }

    struct settings settings;
    
    memset(&settings, 0, sizeof(settings));

    cmd command = process_options(argc, argv, &settings);

    // open input
    // load input

    return dispatch(command, &settings);
}

int dispatch(int command, struct settings *settings)
{
    int result = 0;
    
    switch(command)
    {
        case SHOW_HELP:
            fprintf(stdout, "help\n");
            break;
        case SHOW_VERSION:
            fprintf(stdout, "version information\n");
            break;
        case SHOW_WARRANTY:
            fprintf(stdout, "warranty information\n");
            break;
        case ENTER_INTERACTIVE:
            fprintf(stdout, "interactive mode\n");
            break;
        case EVAL_PATH:
            fprintf(stdout, "evaluating path: \"%s\"\n", settings->json_path);
            break;
        default:
            fprintf(stderr, "panic: unknown command state! this should not happen.\n");
            result = -1;
            break;
    }

    return result;
}
