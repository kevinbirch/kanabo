
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>

#include "options.h"

static struct option options[] = 
{
    // meta commands:
    {"version",     no_argument,       NULL, 'v'}, // print version and exit
    {"no-warranty", no_argument,       NULL, 'w'}, // print no-warranty and exit
    {"help",        no_argument,       NULL, 'h'}, // print help and exit
    // operating modes:
    {"interactive", no_argument,       NULL, 'I'}, // enter interactive mode (requres -F)
    {"path",        required_argument, NULL, 'P'}, // evaluate given path expression and exit
    {"shell",       required_argument, NULL, 'S'}, // transform input into expressions for the given shell (the default mode)
    // optional input file:
    {"file",        required_argument, NULL, 'F'}, // read JSON/YAML input from file instead of stdin
    {0, 0, 0, 0}
};

int process_options(int argc, char **argv, struct settings *settings)
{
    int opt;
    int command = EMIT_SHELL;
    bool done = false;
    bool mode_set = false;
    
    while(!done && (opt = getopt_long(argc, argv, "vwhIP:S:F:", options, NULL)) != -1)
    {
        switch(opt)
        {
            case 'v':
#define ENSURE_COMMAND_ORTHOGONALITY(test) \
                if((test))                 \
                {                          \
                    command = SHOW_HELP;   \
                    done = true;           \
                    break;                 \
                }
                ENSURE_COMMAND_ORTHOGONALITY(1 < argc);
                command = SHOW_VERSION;
                done = true;
                break;
            case 'h':
                ENSURE_COMMAND_ORTHOGONALITY(1 < argc);
                command = SHOW_HELP;
                done = true;
                break;
            case 'w':
                ENSURE_COMMAND_ORTHOGONALITY(1 < argc);
                command = SHOW_WARRANTY;
                done = true;
                break;
            case 'I':
                ENSURE_COMMAND_ORTHOGONALITY(mode_set);
                command = ENTER_INTERACTIVE;
                mode_set = true;
                break;
            case 'P':
                ENSURE_COMMAND_ORTHOGONALITY(mode_set);
                command = EVAL_PATH;
                mode_set = true;
                break;
            case 'S':
                ENSURE_COMMAND_ORTHOGONALITY(mode_set);
                command = EMIT_SHELL;
                mode_set = true;
                break;
            case 'F':
                settings->input_file_name = (char *)malloc(MAXPATHLEN + 1);
                if(NULL == settings->input_file_name)
                {
                    perror(argv[0]);
                    command = ERROR;
                    done = true;
                    break;
                }
                settings->input_file_name[MAXPATHLEN] = 0;
                strncpy(settings->input_file_name, optarg, MAXPATHLEN);
                break;
            case ':':
            case '?':
            default:
                command = SHOW_HELP;
                done = true;
                break;
        }
    }
    
    return command;
}

