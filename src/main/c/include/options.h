#pragma once

#include "str.h"

enum emit_mode
{
    BASH,
    ZSH,
    JSON,
    YAML
};

enum command
{
    SHOW_VERSION,
    SHOW_WARRANTY,
    SHOW_HELP,
    INTERACTIVE_MODE,
    EXPRESSION_MODE
};

enum duplicate_key_strategy
{
    DUPE_CLOBBER,
    DUPE_FAIL
};

typedef enum duplicate_key_strategy DuplicateKeyStrategy;

struct options
{
    const char          *input_file_name;
    const char          *expression;
    enum command         mode;
    enum emit_mode       emit_mode;
    DuplicateKeyStrategy duplicate_strategy;
};

void process_options(const int argc, char * const *argv, struct options *options);

int32_t     parse_duplicate_strategy(const char *value);
const char *duplicate_strategy_name(DuplicateKeyStrategy value);

int32_t     parse_emit_mode(const char *valie);
const char *emit_mode_name(enum emit_mode value);
