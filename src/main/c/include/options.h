
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

typedef enum emit_mode emit_mode;

struct settings
{
    emit_mode  emit_mode;
    const char *json_path;
    const char *input_file_name;
};

enum
{
    SHOW_VERSION,
    SHOW_WARRANTY,
    SHOW_HELP,
    ENTER_INTERACTIVE,
    EVAL_PATH
};

typedef int cmd;

cmd process_options(const int argc, char * const *argv, struct settings *settings);

#endif
