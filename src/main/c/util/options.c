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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>

#include "options.h"

static inline int32_t process_emit_mode(const char *argument);
static inline int32_t process_dupe_strategy(const char *argument);
static inline const char *process_program_name(const char *argv0);

static struct option options[] = 
{
    // meta commands:
    {"version",     no_argument,       NULL, 'v'}, // print version and exit
    {"no-warranty", no_argument,       NULL, 'w'}, // print no-warranty and exit
    {"help",        no_argument,       NULL, 'h'}, // print help and exit
    // operating modes:
    {"interactive", no_argument,       NULL, 'i'}, // enter interactive mode (requres -f/--file), specify the shell to emit for
    {"query",       required_argument, NULL, 'q'}, // evaluate given expression and exit
    // optional arguments:
    {"file",        required_argument, NULL, 'f'}, // read input from file instead of stdin (required for interactive mode)
    {"output",      required_argument, NULL, 'o'}, // emit expressions for the given shell (the default is Bash)
    {"duplicate",   required_argument, NULL, 'd'}, // how to respond to duplicate mapping keys
    {0, 0, 0, 0}
};

#define ENSURE_COMMAND_ORTHOGONALITY(test) \
    if((test))                             \
    {                                      \
        settings->command = SHOW_HELP;     \
        done = true;                       \
        break;                             \
    }

enum command process_options(const int argc, char * const *argv, struct settings *settings)
{
    int opt;
    int32_t mode = -1;
    int32_t strategy = -1;
    bool done = false;
    bool interaction_decided = false;
    
    settings->program_name = process_program_name(argv[0]);
    settings->emit_mode = BASH;
    settings->duplicate_strategy = DUPE_CLOBBER;
    settings->expression = NULL;
    settings->input_file_name = NULL;

    while(!done && (opt = getopt_long(argc, argv, "vwhiq:o:f:d:", options, NULL)) != -1)
    {
        switch(opt)
        {
            case 'v':
                ENSURE_COMMAND_ORTHOGONALITY(2 < argc);
                settings->command = SHOW_VERSION;
                done = true;
                break;
            case 'h':
                ENSURE_COMMAND_ORTHOGONALITY(2 < argc);
                settings->command = SHOW_HELP;
                done = true;
                break;
            case 'w':
                ENSURE_COMMAND_ORTHOGONALITY(2 < argc);
                settings->command = SHOW_WARRANTY;
                done = true;
                break;
            case 'i':
                ENSURE_COMMAND_ORTHOGONALITY(interaction_decided);
                settings->command = INTERACTIVE_MODE;
                interaction_decided = true;
                break;
            case 'q':
                ENSURE_COMMAND_ORTHOGONALITY(interaction_decided);
                settings->command = EXPRESSION_MODE;
                settings->expression = make_string(optarg);
                interaction_decided = true;
                break;
            case 'o':
                mode = process_emit_mode(optarg);
                if(-1 == mode)
                {
                    fprintf(stderr, "%s: unsupported output format `%s'\n", settings->program_name, optarg);
                    settings->command = SHOW_HELP;
                    done = true;
                }
                settings->emit_mode = (enum emit_mode)mode;
                break;
            case 'f':
                settings->input_file_name = optarg;
                break;
            case 'd':
                strategy = process_dupe_strategy(optarg);
                if(-1 == strategy)
                {
                    fprintf(stderr, "%s: unsupported duplicate strategy `%s'\n", settings->program_name, optarg);
                    settings->command = SHOW_HELP;
                    done = true;
                }
                settings->duplicate_strategy = (enum loader_duplicate_key_strategy)strategy;
                break;
            case ':':
            case '?':
            default:
                settings->command = SHOW_HELP;
                done = true;
                break;
        }
    }

    if(!interaction_decided)
    {
        fprintf(stderr, "%s: either `--query <expression>' or `--interactive' must be specified.\n", settings->program_name);
        settings->command = SHOW_HELP;
    }
    else if(INTERACTIVE_MODE == settings->command && NULL == settings->input_file_name)
    {
        fprintf(stderr, "%s: using `--interactive' requires `--file <filename>' must also be specified.\n", settings->program_name);
        settings->command = SHOW_HELP;
    }

    return settings->command;
}

static inline int32_t process_emit_mode(const char *argument)
{
    if(strncmp("bash", argument, 4) == 0)
    {
        return BASH;
    }
    else if(strncmp("zsh", argument, 3) == 0)
    {
        return ZSH;
    }
    else if(strncmp("json", argument, 4) == 0)
    {
        return JSON;
    }
    else if(strncmp("yaml", argument, 4) == 0)
    {
        return YAML;
    }
    else
    {
        return -1;
    }
}

static inline int32_t process_dupe_strategy(const char *argument)
{
    if(0 == strncmp("clobber", argument, 7ul))
    {
        return DUPE_CLOBBER;
    }
    else if(0 == strncmp("warn", argument, 4ul))
    {
        return DUPE_WARN;
    }
    else if(0 == strncmp("fail", argument, 4ul))
    {
        return DUPE_FAIL;
    }
    else
    {
        return -1;
    }
}

static inline const char *process_program_name(const char *argv0)
{
    char *slash = strrchr(argv0, '/');
    return NULL == slash ? argv0 : slash + 1;
}
