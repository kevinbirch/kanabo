
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "options.h"
#include "shell.h"

int dispatch(int command, struct settings *settings);

int main(int argc, char **argv)
{
    struct settings settings;
    
    memset(&settings, 0, sizeof(settings));

    int command = process_options(argc, argv, &settings);

    argc -= optind;
    argv += optind;

    dispatch(command, &settings);
    
    // open input
    // load input
    // dispatch command

    return 0;
}

int dispatch(int command, struct settings *settings)
{
    int result = 0;
    
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
            if(settings->input_file_name)
            {
                fprintf(stdout, "using input file: %s\n", settings->input_file_name);
            }
            break;
        case EVAL_PATH:
            fprintf(stdout, "json path evaluation\n");
            if(settings->input_file_name)
            {
                fprintf(stdout, "using input file: %s\n", settings->input_file_name);
            }
            break;
        case EMIT_SHELL:
            fprintf(stdout, "shell expressions\n");
            if(settings->input_file_name)
            {
                fprintf(stdout, "using input file: %s\n", settings->input_file_name);
            }
            break;
        case ERROR:
            result = ERROR;
            break;
        default:
            fprintf(stderr, "panic: unknown command state! this should not happen.\n");
            result = -1;
            break;
    }

    return result;
}
