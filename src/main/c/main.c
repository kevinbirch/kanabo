
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "options.h"
#include "shell.h"

void dispatch(int command, struct settings *settings);

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

    dispatch(command, &settings);
    
    // open input
    // load input
    // dispatch command

    return 0;
}

void dispatch(int command, struct settings *settings)
{
    if(settings->input_file_name)
    {
        fprintf(stdout, "using input file: %s\n", settings->input_file_name);
    }
    switch(settings->emit_mode)
    {
        case BASH:
            fprintf(stdout, "using bash mode\n");
            break;
        case ZSH:
            fprintf(stdout, "using zsh mode\n");
            break;
    }
    
    switch(command)
    {
        case SHOW_VERSION:
            fprintf(stdout, "version information\n");
            break;
        case SHOW_WARRANTY:
            fprintf(stdout, "warranty information\n");
            break;
        case SHOW_HELP:
            fprintf(stdout, "help\n");
            break;
        case ENTER_INTERACTIVE:
            fprintf(stdout, "interactive mode\n");
            break;
        case EVAL_PATH:
            fprintf(stdout, "single path evaluation\n");
            fprintf(stdout, "evaluating path: \"%s\"\n", settings->json_path);
            break;
        default:
            fprintf(stderr, "panic: unknown command state! this should not happen.\n");
            // we should exit ungracefully here?
            break;
    }
}
