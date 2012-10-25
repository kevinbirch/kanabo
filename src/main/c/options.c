
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>

#include "options.h"

emit_mode process_emit_mode(const char *argument);
const char *program_name(const char *argv0);

static struct option options[] = 
{
    // meta commands:
    {"version",     no_argument,       NULL, 'v'}, // print version and exit
    {"no-warranty", no_argument,       NULL, 'w'}, // print no-warranty and exit
    {"help",        no_argument,       NULL, 'h'}, // print help and exit
    // operating modes:
    {"interactive", no_argument,       NULL, 'I'}, // enter interactive mode (requres -f/--file), specify the shell to emit for
    {"path",        required_argument, NULL, 'P'}, // evaluate given path expression and exit (the default mode, with a path of "$")
    // optional arguments:
    {"file",        required_argument, NULL, 'f'}, // read input from file instead of stdin (required for interactive mode)
    {"shell",       required_argument, NULL, 's'}, // emit expressions for the given shell (the default is Bash)
    {0, 0, 0, 0}
};

cmd process_options(const int argc, char * const *argv, struct settings *settings)
{
    int opt;
    int command = EVAL_PATH;
    bool done = false;
    bool mode_set = false;
    
    settings->emit_mode = BASH;
    settings->json_path = "$";
    settings->input_file_name = NULL;

    while(!done && (opt = getopt_long(argc, argv, "vwhIP:s:f:", options, NULL)) != -1)
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
                settings->json_path = optarg;
                mode_set = true;
                break;
            case 's':
                settings->emit_mode = process_emit_mode(optarg);
                if(-1 == settings->emit_mode)
                {
                    fprintf(stderr, "%s: unsupported shell mode `%s'\n", program_name(argv[0]), optarg);
                    command = SHOW_HELP;
                    done = true;
                }
                break;
            case 'f':
                settings->input_file_name = optarg;
                break;
            case ':':
            case '?':
            default:
                command = SHOW_HELP;
                done = true;
                break;
        }
    }

    if(ENTER_INTERACTIVE == command && NULL == settings->input_file_name)
    {
        command = SHOW_HELP;
    }
    
    return command;
}

emit_mode process_emit_mode(const char *argument)
{
    if(strncmp("bash", argument, 4) == 0)
    {
        return BASH;
    }
    else if(strncmp("zsh", argument, 3) == 0)
    {
        return ZSH;
    }
    else
    {
        return -1;
    }
}

const char *program_name(const char *argv0)
{
    char *result = strrchr(argv0, '/');
    return NULL == result ? argv0 : result + 1;
}
