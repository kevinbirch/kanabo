
#ifndef OPTIONS_H
#define OPTIONS_H

#include <getopt.h>

struct section
{
    char *name;
    char *description;
    int *options;
};

struct help
{
    char *option;
    char *description;
};

enum emit_mode
{
    BASH,
    ZSH
};

typedef enum emit_mode emit_t;

struct settings
{
    bool interactive;
    emit_t emit_mode;
    char *input_file_name;
};
    
enum
{
    SHOW_VERSION,
    SHOW_WARRANTY,
    SHOW_HELP,
    ENTER_INTERACTIVE,
    EVAL_PATH,
    EMIT_SHELL,
    ERROR = -1
};

int process_options(int argc, char **argv, struct settings *settings);

#endif
