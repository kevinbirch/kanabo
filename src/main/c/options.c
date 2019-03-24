#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>
#include <getopt.h>

#include "options.h"

typedef struct option argument;

static const char * const EMIT_MODES [] =
{
    [BASH] = "bash",
    [ZSH] = "zsh",
    [JSON] = "json",
    [YAML] = "yaml"
};

static const char * const DUPLICATE_STRATEGIES[] =
{
    [DUPE_CLOBBER] = "clobber",
    [DUPE_FAIL] = "fail"
};

static argument arguments[] =
{
    // meta commands:
    {"version",     no_argument,       NULL, 'v'}, // print version and exit
    {"no-warranty", no_argument,       NULL, 'w'}, // print no-warranty and exit
    {"help",        no_argument,       NULL, 'h'}, // print help and exit
    // operating modes:
    {"query",       required_argument, NULL, 'q'}, // evaluate given expression and exit
    // optional arguments:
    {"output",      required_argument, NULL, 'o'}, // emit expressions for the given shell
    {"duplicate",   required_argument, NULL, 'd'}, // how to respond to duplicate mapping keys
    {0, 0, 0, 0}
};

#define ENSURE_COMMAND_ORTHOGONALITY(test) \
    if((test))                             \
    {                                      \
        options->mode = SHOW_HELP;     \
        done = true;                       \
        break;                             \
    }

int32_t parse_emit_mode(const char *value)
{
    if(strncmp("bash", value, 4) == 0)
    {
        return BASH;
    }
    else if(strncmp("zsh", value, 3) == 0)
    {
        return ZSH;
    }
    else if(strncmp("json", value, 4) == 0)
    {
        return JSON;
    }
    else if(strncmp("yaml", value, 4) == 0)
    {
        return YAML;
    }
    else
    {
        return -1;
    }
}

const char * emit_mode_name(enum emit_mode value)
{
    return EMIT_MODES[value];
}

int32_t parse_duplicate_strategy(const char *value)
{
    if(0 == strncmp("clobber", value, 7ul))
    {
        return DUPE_CLOBBER;
    }
    else if(0 == strncmp("fail", value, 4ul))
    {
        return DUPE_FAIL;
    }
    else
    {
        return -1;
    }
}

const char * duplicate_strategy_name(DuplicateKeyStrategy value)
{
    return DUPLICATE_STRATEGIES[value];
}

void process_options(const int argc, char * const *argv, struct options *options)
{
    int opt;
    bool done = false;

    options->emit_mode = JSON;
    options->duplicate_strategy = DUPE_CLOBBER;
    options->input_file_name = NULL;
    options->mode = INTERACTIVE_MODE;

    while(!done && (opt = getopt_long(argc, argv, "vwhq:o:d:", arguments, NULL)) != -1)
    {
        switch(opt)
        {
            case 'v':
                ENSURE_COMMAND_ORTHOGONALITY(2 < argc);
                options->mode = SHOW_VERSION;
                done = true;
                break;
            case 'h':
                ENSURE_COMMAND_ORTHOGONALITY(2 < argc);
                options->mode = SHOW_HELP;
                done = true;
                break;
            case 'w':
                ENSURE_COMMAND_ORTHOGONALITY(2 < argc);
                options->mode = SHOW_WARRANTY;
                done = true;
                break;
            case 'q':
                options->mode = EXPRESSION_MODE;
                options->expression = optarg;
                options->mode = EXPRESSION_MODE;
                break;
            case 'o':
            {
                int32_t mode = parse_emit_mode(optarg);
                if(-1 == mode)
                {
                    fprintf(stderr, "error: unsupported output format \"%s\"\n", optarg);
                    options->mode = SHOW_HELP;
                    done = true;
                    break;
                }
                options->emit_mode = (enum emit_mode)mode;
                break;
            }
            case 'd':
            {
                int32_t strategy = parse_duplicate_strategy(optarg);
                if(-1 == strategy)
                {
                    fprintf(stderr, "error: unsupported duplicate strategy \"%s\"\n", optarg);
                    options->mode = SHOW_HELP;
                    done = true;
                    break;
                }
                options->duplicate_strategy = (DuplicateKeyStrategy)strategy;
                break;
            }
            case ':':
            case '?':
            default:
                options->mode = SHOW_HELP;
                done = true;
                break;
        }
    }

    if(optind > argc)
    {
        fputs("error: some command line options were not processed\n", stderr);
        options->mode = SHOW_HELP;
        return;
    }

    if(argc - optind)
    {
        options->input_file_name = argv[optind];
    }

    if(INTERACTIVE_MODE == options->mode &&
       NULL != options->input_file_name &&
       1 == strlen(options->input_file_name) &&
       '-' == options->input_file_name[0])
    {
        fputs("error: the standard input shortcut "-" can't be used with interactive evaluation\n", stderr);
        options->mode = SHOW_HELP;
    }
    else if(EXPRESSION_MODE == options->mode && NULL == options->input_file_name)
    {
        fputs("error: an input filename (or "-") must be provided for single expression evaluation\n", stderr);
        options->mode = SHOW_HELP;
    }
}
