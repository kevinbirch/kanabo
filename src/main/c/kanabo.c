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

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/errno.h>
#include <string.h>
#include <ctype.h>

#include "warranty.h"
#include "options.h"
#include "loader.h"
#include "jsonpath.h"
#include "evaluator.h"
#include "emit.h"
#include "log.h"
#include "version.h"
#include "linenoise.h"

static const char * const DEFAULT_PROMPT = ">> ";
static const char * const BANNER =
    "kanabo " VERSION " (built: " BUILD_DATE ")\n"
    "[" BUILD_COMPILER "] on " BUILD_HOSTNAME "\n";
static const char * const INTERACTIVE_HELP =
    "The following commands can be used, any other input is treated as JSONPath.\n"
    "\n"
    ":load <path>             Load JSON/YAML data from the file <path>.\n"
    ":output [<format>]       Get/set the output format. (`bash', `zsh', `json' and `yaml' are supported).\n"
    ":duplicate [<strategy>]  Get/set the strategy to handle duplicate mapping keys (`clobber' (default), `warn' or `fail').\n";

#define is_stdin_filename(NAME) \
    0 == memcmp("-", (NAME), 1)

#define use_stdin(NAME) \
    NULL == (NAME) || is_stdin_filename((NAME))

#define get_input_name(NAME) \
    use_stdin((NAME)) ? "stdin" : (NAME)

struct context
{
    struct options *options;
    document_model *model;
};

__attribute__((__format__ (__printf__, 2, 3)))
static void error(const struct options *options, const char *format, ...)
{
    va_list rest;
    if(INTERACTIVE_MODE != options->mode)
    {
        fprintf(stderr, "%s: ", options->program_name);
    }
    va_start(rest, format);
    vfprintf(stderr, format, rest);
    va_end(rest);
    fprintf(stderr, "\n");
}

static jsonpath *parse_expression(const struct options *options, const char *expression)
{
    log_trace(options->program_name, "parsing expression");
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    if(NULL == parser)
    {
        error(options, "while parsing the expression '%s': %s", expression, strerror(errno));
        return NULL;
    }

    if(parser_status(parser))
    {
        char *message = parser_status_message(parser);
        error(options, "while parsing the expression '%s': %s", expression, message);
        free(message);
        parser_free(parser);
        return NULL;
    }

    jsonpath *path = parse(parser);
    if(parser_status(parser))
    {
        char *message = parser_status_message(parser);
        error(options, "while parsing the expression '%s': %s", expression, message);
        free(message);
        path_free(path);
        path = NULL;
    }

    parser_free(parser);
    return path;
}

static nodelist *evaluate_expression(const struct options *options, const document_model *model, const jsonpath *path)
{
    log_trace(options->program_name, "evaluating expression");
    evaluator_context *evaluator = make_evaluator(model, path);
    if(NULL == evaluator)
    {
        char *expression = (char *)path_expression(path);
        error(options, "while evaluating the expression '%s': %s", expression, strerror(errno));
        return NULL;
    }

    if(evaluator_status(evaluator))
    {
        char *expression = (char *)path_expression(path);
        const char *message = evaluator_status_message(evaluator);
        error(options, "while evaluating the expression '%s': %s", expression, message);
        evaluator_free(evaluator);
        return NULL;
    }

    nodelist *list = evaluate(evaluator);
    if(evaluator_status(evaluator))
    {
        char *expression = (char *)path_expression(path);
        const char *message = evaluator_status_message(evaluator);
        error(options, "while evaluating the expression '%s': %s", expression, message);
        nodelist_free(list);
        list = NULL;
    }

    evaluator_free(evaluator);
    return list;
}

static emit_function get_emitter(const struct options *options)
{
    emit_function result = NULL;
    switch(options->emit_mode)
    {
        case BASH:
            log_debug(options->program_name, "using bash emitter");
            result = emit_bash;
            break;
        case ZSH:
            log_debug(options->program_name, "using zsh emitter");
            result = emit_zsh;
            break;
        case JSON:
            log_debug(options->program_name, "using json emitter");
            result = emit_json;
            break;
        case YAML:
            log_debug(options->program_name, "using yaml emitter");
            result = emit_yaml;
            break;
    }

    return result;
}

static int apply_expression(const struct options *options, document_model *model, const char *expression)
{
    jsonpath *path = parse_expression(options, expression);
    if(NULL == path)
    {
        return EXIT_FAILURE;
    }

    nodelist *list = evaluate_expression(options, model, path);
    if(NULL == list)
    {
        path_free(path);
        return EXIT_FAILURE;
    }

    emit_function emit = get_emitter(options);
    if(NULL == emit)
    {
        path_free(path);
        nodelist_free(list);
        return EXIT_FAILURE;
    }

    emit(list, options);

    path_free(path);
    nodelist_free(list);

    return EXIT_SUCCESS;
}

static FILE *open_input(const struct options *options)
{
    if(use_stdin(options->input_file_name))
    {
        log_debug(options->program_name, "reading from stdin");
        return stdin;
    }
    else
    {
        log_debug(options->program_name, "reading from file: '%s'", options->input_file_name);
        return fopen(options->input_file_name, "r");
    }
}

static void close_input(const struct options *options, FILE *input)
{
    if(stdin == input)
    {
        return;
    }
    log_trace(options->program_name, "closing input file");

    errno = 0;
    if(fclose(input))
    {
        const char *name = get_input_name(options->input_file_name);
        error(options, "while reading '%s': %s", name, strerror(errno));
    }
}

#define ensure_object(OBJ)                                              \
    if(NULL == (OBJ))                                                   \
    {                                                                   \
        const char *name = get_input_name(options->input_file_name);    \
        error(options, "while reading '%s': %s", name, strerror(errno)); \
        return NULL;                                                    \
    }

#define ensure_loader(LOADER)                                   \
    ensure_object((LOADER));                                    \
    if(loader_status((LOADER)))                                 \
    {                                                           \
        close_input(options, input);                            \
        const char *name = get_input_name(options->input_file_name);    \
        char *message = loader_status_message((LOADER));        \
        error(options, "while reading '%s': %s", name, message);  \
        free(message);                                          \
        loader_free(loader);                                    \
        return NULL;                                            \
    }

static loader_context *prepare_loader(const struct options *options, FILE *input)
{
    log_trace(options->program_name, "creating loader...");
    loader_context *loader = make_file_loader(input);
    ensure_loader(loader);
    loader_set_dupe_strategy(loader, options->duplicate_strategy);

    return loader;
}

#define ensure_load(LOADER)                                     \
    if(loader_status((LOADER)))                                 \
    {                                                           \
        const char *name = get_input_name(options->input_file_name);    \
        char *message = loader_status_message((LOADER));        \
        error(options, "while reading '%s': %s", name, message);  \
        free(message);                                          \
        loader_free(loader);                                    \
        return NULL;                                            \
    }

static document_model *load_model(const struct options *options)
{
    FILE *input = open_input(options);
    ensure_object(input);

    loader_context *loader = prepare_loader(options, input);
    if(NULL == loader)
    {
        return NULL;
    }

    log_trace(options->program_name, "loading model...");
    document_model *model = load(loader);
    ensure_load(loader);

    close_input(options, input);
    loader_free(loader);
    log_trace(options->program_name, "model loaded.");

    return model;
}

static void output_command(struct context *context, const char *argument)
{
    log_debug(context->options->program_name, "processing output command...");
    if(!argument)
    {
        log_trace(context->options->program_name, "no command argument, printing current value");
        fprintf(stdout, "%s\n", emit_mode_name(context->options->emit_mode));
        return;
    }

    int32_t mode = parse_emit_mode(argument);
    if(-1 == mode)
    {
        error(context->options, "unsupported output format `%s'", argument);
    }

    log_debug(context->options->program_name, "setting value to: %s", argument);
    context->options->emit_mode = (enum emit_mode)mode;
}

static void duplicate_command(struct context *context, const char *argument)
{
    log_debug(context->options->program_name, "processing duplicate command...");
    if(!argument)
    {
        log_trace(context->options->program_name, "no command argument, printing current value");
        fprintf(stdout, "%s\n", duplicate_strategy_name(context->options->duplicate_strategy));
        return;
    }

    int32_t strategy = parse_duplicate_strategy(argument);
    if(-1 == strategy)
    {
        error(context->options, "unsupported duplicate stratety `%s'", argument);
    }

    log_debug(context->options->program_name, "setting value to: %s", argument);
    context->options->duplicate_strategy = (enum loader_duplicate_key_strategy)strategy;
}

static void load_command(struct context *context, const char *argument)
{
    log_debug(context->options->program_name, "processing load command...");
    if(!argument)
    {
        log_trace(context->options->program_name, "no command argument, aborting...");
        error(context->options, ":load command requires an argument");
        return;
    }

    log_debug(context->options->program_name, "found command argument, loading '%s'...", argument);
    context->options->input_file_name = argument;
    if(context->model)
    {
        model_free(context->model);
    }
    context->model = load_model(context->options);
}

static const char *get_argument(const char *command)
{
    char *arg = (char *)command;
    // find end of command
    while(!isspace(*arg) && '\0' != *arg)
    {
        arg++;
    }
    // there is no spoon
    if('\0' == *arg)
    {
        return NULL;
    }

    // strip leading whitespace
    while('\0' != *arg && isspace(*arg))
    {
        arg++;
    }
    // still no spoon
    if('\0' == *arg)
    {
        return NULL;
    }
    // oh wait, I found the spoon over here
    return arg;
}

static void dispatch_interactive_command(struct context *context, const char *command)
{
    if(0 == memcmp("?", command, 1) || 0 == memcmp(":help", command, 5))
    {
        fwrite(INTERACTIVE_HELP, strlen(INTERACTIVE_HELP), 1, stdout);
        return;
    }
    else if(0 == memcmp(":output", command, 7))
    {
        output_command(context, get_argument(command));
        return;
    }
    else if(0 == memcmp(":duplicate", command, 10))
    {
        duplicate_command(context, get_argument(command));
        return;
    }
    else if(0 == memcmp(":load", command, 5))
    {
        load_command(context, get_argument(command));
        return;
    }
    else
    {
        if(NULL == context->model)
        {
            error(context->options, "no input loaded, use the `:load' command");
            return;
        }
        apply_expression(context->options, context->model, command);
    }
}

static void tty_interctive_mode(struct context *context)
{
    log_debug(context->options->program_name, "entering tty interative mode");

    fwrite(BANNER, strlen(BANNER), 1, stdout);
    char *prompt = (char *)DEFAULT_PROMPT;

    char *input;
    while(true)
    {
        input = linenoise(prompt);
        if(NULL == input)
        {
            break;
        }
        if('\0' == input[0])
        {
            free(input);
            continue;
        }

        linenoiseHistoryAdd(input);
        dispatch_interactive_command(context, input);
        free(input);
    }
}


static void pipe_interactive_model(struct context *context)
{
    char *input= NULL;
    size_t len = 0;
    ssize_t read;

    log_debug(context->options->program_name, "entering non-tty interative mode");
    while((read = getline(&input, &len, stdin)) != -1)
    {
        if(0 == read || '\n' == input[0])
        {
            continue;
        }
        input[read - 1] = '\0';
        dispatch_interactive_command(context, input);
        fprintf(stdout, "EOD\n");
        fflush(stdout);
    }
    free(input);
}

static int interactive_mode(struct options *options)
{
    struct context context;
    memset(&context, 0, sizeof(struct context));
    if(options->input_file_name)
    {
        context.model = load_model(options);
    }
    context.options = options;

    if(isatty(fileno(stdin)))
    {
        tty_interctive_mode(&context);
    }
    else
    {
        pipe_interactive_model(&context);
    }
    model_free(context.model);

    return EXIT_SUCCESS;
}

static int expression_mode(const struct options *options)
{
    document_model *model = load_model(options);
    if(NULL == model)
    {
        return EXIT_FAILURE;
    }

    log_debug(options->program_name, "evaluating expression: \"%s\"", options->expression);
    int result = apply_expression(options, model, options->expression);
    model_free(model);

    return result;
}

static int execute_mode(enum command cmd, struct options *options)
{
    int result = EXIT_SUCCESS;

    switch(cmd)
    {
        case SHOW_HELP:
            fprintf(stdout, "%s", HELP);
            break;
        case SHOW_VERSION:
            fprintf(stdout, "kanabo %s\n", VERSION);
            break;
        case SHOW_WARRANTY:
            fprintf(stdout, "%s", NO_WARRANTY);
            break;
        case INTERACTIVE_MODE:
            result = interactive_mode(options);
            break;
        case EXPRESSION_MODE:
            result = expression_mode(options);
            break;
    }

    return result;
}

static int run(const int argc, char * const *argv)
{
    struct options options;
    memset(&options, 0, sizeof(struct options));
    enum command cmd = process_options(argc, argv, &options);

    return execute_mode(cmd, &options);
}

int main(const int argc, char * const *argv)
{
    if(NULL == argv || NULL == argv[0])
    {
        fprintf(stderr, "error: whoa! something is wrong, there are no program arguments.\n");
        return EXIT_FAILURE;
    }

    enable_logging();
    set_log_level_from_env();

    return run(argc, argv);
}
