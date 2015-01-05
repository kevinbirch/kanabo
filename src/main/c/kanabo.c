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
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <execinfo.h>
#include <libgen.h>

#include "warranty.h"
#include "options.h"
#include "loader.h"
#include "jsonpath.h"
#include "evaluator.h"
#include "emit.h"
#include "log.h"
#include "version.h"
#include "linenoise.h"

static const char * const DEFAULT_PROGRAM_NAME = "kanabo";

static const char * const HELP =
    "usage: kanabo [-o <format>] [-d <strategy>] -q <jsonpath> [<file> | '-']\n"
    "       kanabo [-o <format>] [-d <strategy>] [<file>]\n"
    "\n"
    "OPTIONS:\n"
    "-q, --query <jsonpath>      Specify a single JSONPath query to execute against the input document and exit.\n"
    "-o, --output <format>       Specify the output format (`bash' (default), `zsh', `json' or `yaml').\n"
    "-d, --duplicate <strategy>  Specify how to handle duplicate mapping keys (`clobber' (default), `warn' or `fail').\n"
    "\n"
    "STANDALONE OPTIONS:\n"
    "-v, --version               Print the version information and exit.\n"
    "-w, --no-warranty           Print the no-warranty information and exit.\n"
    "-h, --help                  Print the usage summary and exit.\n";

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

static const char *program_name = NULL;
static bool is_interactive = false;

#define kanabo_debug(FORMAT, ...) log_debug(program_name, (FORMAT), ##__VA_ARGS__)
#define kanabo_trace(FORMAT, ...) log_trace(program_name, (FORMAT), ##__VA_ARGS__)


__attribute__((__format__ (__printf__, 1, 2)))
static void error(const char *format, ...)
{
    va_list rest;
    if(is_interactive)
    {
        fputs(program_name, stderr);
        fputs(": ", stderr);
    }
    va_start(rest, format);
    vfprintf(stderr, format, rest);
    va_end(rest);
    fputc('\n', stderr);
}

static jsonpath *parse_expression(const char *expression)
{
    kanabo_trace("parsing expression");
    parser_context *parser = make_parser((uint8_t *)expression, strlen(expression));
    if(NULL == parser)
    {
        error("while parsing the expression '%s': %s", expression, strerror(errno));
        return NULL;
    }

    if(parser_status(parser))
    {
        char *message = parser_status_message(parser);
        error("while parsing the expression '%s': %s", expression, message);
        free(message);
        parser_free(parser);
        return NULL;
    }

    jsonpath *path = parse(parser);
    if(parser_status(parser))
    {
        char *message = parser_status_message(parser);
        error("while parsing the expression '%s': %s", expression, message);
        free(message);
        path_free(path);
        path = NULL;
    }

    parser_free(parser);
    return path;
}

static nodelist *evaluate_expression(const jsonpath *path, const DocumentModel *model)
{
    kanabo_trace("evaluating expression");
    MaybeNodelist maybe = evaluate(model, path);
    if(NOTHING == maybe.tag)
    {
        char *expression = (char *)path_expression(path);
        error("while evaluating the expression '%s': %s", expression, maybe.nothing.message);
        return NULL;
    }
    return maybe.just;
}

static emit_function get_emitter(enum emit_mode emit_mode)
{
    emit_function result = NULL;
    switch(emit_mode)
    {
        case BASH:
            kanabo_debug("using bash emitter");
            result = emit_bash;
            break;
        case ZSH:
            kanabo_debug("using zsh emitter");
            result = emit_zsh;
            break;
        case JSON:
            kanabo_debug("using json emitter");
            result = emit_json;
            break;
        case YAML:
            kanabo_debug("using yaml emitter");
            result = emit_yaml;
            break;
    }

    return result;
}

static int apply_expression(const char *expression, DocumentModel *model, enum emit_mode emit_mode)
{
    kanabo_debug("evaluating expression: \"%s\"", expression);
    jsonpath *path = parse_expression(expression);
    if(NULL == path)
    {
        return EXIT_FAILURE;
    }

    nodelist *list = evaluate_expression(path, model);
    if(NULL == list)
    {
        path_free(path);
        return EXIT_FAILURE;
    }

    emit_function emitter = get_emitter(emit_mode);
    if(!emitter(list))
    {
        error("unable to emit results");
    }

    path_free(path);
    nodelist_free(list);

    return EXIT_SUCCESS;
}

static FILE *open_input(const char *input_file_name)
{
    if(use_stdin(input_file_name))
    {
        kanabo_debug("reading from stdin");
        return stdin;
    }
    else
    {
        kanabo_debug("reading from file: '%s'", input_file_name);
        errno = 0;
        return fopen(input_file_name, "r");
    }
}

static void close_input(FILE *input)
{
    if(stdin == input)
    {
        return;
    }

    kanabo_trace("closing input file");
    errno = 0;
    fclose(input);
}

static DocumentModel *load_document(const char *input_file_name, dup_strategy strategy)
{
    FILE *input = open_input(input_file_name);
    if(NULL == input)
    {
        const char *name = get_input_name(input_file_name);
        error("while reading '%s': %s", name, strerror(errno));
        return NULL;
    }

    MaybeDocument maybe = load_file(input, strategy);
    close_input(input);
    if(NOTHING == maybe.tag)
    {
        const char *name = get_input_name(input_file_name);
        error("while reading '%s': %s", name, maybe.nothing.message);
        free(maybe.nothing.message);

        return NULL;
    }
    else
    {
        return maybe.just;
    }
}

static void output_command(const char *argument, struct options *options)
{
    kanabo_debug("processing output command...");
    if(!argument)
    {
        kanabo_trace("no command argument, printing current value");
        fputs(emit_mode_name(options->emit_mode), stdout);
        fputc('\n', stdout);
        return;
    }

    int32_t mode = parse_emit_mode(argument);
    if(-1 == mode)
    {
        error("unsupported output format `%s'", argument);
    }

    kanabo_debug("setting value to: %s", argument);
    options->emit_mode = (enum emit_mode)mode;
}

static void duplicate_command(const char *argument, struct options *options)
{
    kanabo_debug("processing duplicate command...");
    if(!argument)
    {
        kanabo_trace("no command argument, printing current value");
        fputs(duplicate_strategy_name(options->duplicate_strategy), stdout);
        fputc('\n', stdout);
        return;
    }

    int32_t strategy = parse_duplicate_strategy(argument);
    if(-1 == strategy)
    {
        error("unsupported duplicate stratety `%s'", argument);
    }

    kanabo_debug("setting value to: %s", argument);
    options->duplicate_strategy = (enum loader_duplicate_key_strategy)strategy;
}

static DocumentModel *load_command(const char *argument, struct options *options)
{
    kanabo_debug("processing load command...");
    if(!argument)
    {
        kanabo_trace("no command argument, aborting...");
        error(":load command requires an argument");
        return NULL;
    }

    kanabo_debug("found command argument, loading '%s'...", argument);
    return load_document(argument, options->duplicate_strategy);
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

static void dispatch_interactive_command(const char *command, struct options *options, DocumentModel **model)
{
    if(0 == memcmp("?", command, 1) || 0 == memcmp(":help", command, 5))
    {
        fwrite(INTERACTIVE_HELP, strlen(INTERACTIVE_HELP), 1, stdout);
    }
    else if(0 == memcmp(":output", command, 7))
    {
        output_command(get_argument(command), options);
    }
    else if(0 == memcmp(":duplicate", command, 10))
    {
        duplicate_command(get_argument(command), options);
    }
    else if(0 == memcmp(":load", command, 5))
    {
        DocumentModel *new_model = load_command(get_argument(command), options);
        if(new_model)
        {
            model_free(*model);
            *model = new_model;
        }
    }
    else
    {
        if(NULL == *model)
        {
            error("no input loaded, use the `:load' command");
            return;
        }
        apply_expression(command, *model, options->emit_mode);
    }
}

static void tty_interctive_mode(struct options *options)
{
    kanabo_debug("entering tty interative mode");

    fwrite(BANNER, strlen(BANNER), 1, stdout);
    char *prompt = (char *)DEFAULT_PROMPT;

    DocumentModel *model = NULL;
    if(options->input_file_name)
    {
        model = load_document(options->input_file_name, options->duplicate_strategy);
    }

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
        dispatch_interactive_command(input, options, &model);
        free(input);
    }
    model_free(model);
}

static void pipe_interactive_mode(struct options *options)
{
    char *input= NULL;
    size_t len = 0;
    ssize_t read;

    DocumentModel *model = NULL;
    if(options->input_file_name)
    {
        model = load_document(options->input_file_name, options->duplicate_strategy);
    }

    kanabo_debug("entering non-tty interative mode");
    while((read = getline(&input, &len, stdin)) != -1)
    {
        if(0 == read || '\n' == input[0])
        {
            continue;
        }
        input[read - 1] = '\0';  // N.B. `read` should always be positive here
        dispatch_interactive_command(input, options, &model);
        fputs("EOD\n", stdout);
        fflush(stdout);
    }
    free(input);
    model_free(model);
}

static int interactive_mode(struct options *options)
{
    is_interactive = true;
    if(isatty(fileno(stdin)))
    {
        tty_interctive_mode(options);
    }
    else
    {
        pipe_interactive_mode(options);
    }

    return EXIT_SUCCESS;
}

static int expression_mode(struct options *options)
{
    DocumentModel *model = load_document(options->input_file_name, options->duplicate_strategy);
    if(NULL == model)
    {
        return EXIT_FAILURE;
    }
    else
    {
        kanabo_trace("model loaded.");

        int result = apply_expression(options->expression, model, options->emit_mode);
        model_free(model);

        return result;
    }
}

static int execute_command(enum command cmd, struct options *options)
{
    int result = EXIT_SUCCESS;

    switch(cmd)
    {
        case SHOW_HELP:
            fputs(HELP, stdout);
            break;
        case SHOW_VERSION:
            fputs("kanabo " VERSION "\n", stdout);
            break;
        case SHOW_WARRANTY:
            fputs(NO_WARRANTY, stdout);
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

    return execute_command(cmd, &options);
}

static void handle_signal(int sigval)
{
    void *stack[20];
    int depth;

    if(SIGSEGV == sigval || SIGABRT == sigval)
    {
        depth = backtrace(stack, 20);
        fprintf(stderr, "Backtrace follows (most recent first):\n");
        backtrace_symbols_fd(stack, depth, fileno(stderr));
        signal(sigval, SIG_DFL);
    }

    raise(sigval);
}

static inline const char *get_program_name(const char *argv0)
{
    char *name = basename((char *)argv0);
    if(NULL == name)
    {
        return DEFAULT_PROGRAM_NAME;
    }
    return name;
}

static void install_handlers(const char * argv0)
{
    if(SIG_ERR == signal(SIGSEGV, handle_signal))
    {
        perror(argv0);
        exit(EXIT_FAILURE);
    }
    if(SIG_ERR == signal(SIGABRT, handle_signal))
    {
        perror(argv0);
        exit(EXIT_FAILURE);
    }
}

int main(const int argc, char * const *argv)
{
    if(1 > argc || NULL == argv || NULL == argv[0])
    {
        fputs("error: whoa! something is wrong, there are no program arguments.\n", stderr);
        return EXIT_FAILURE;
    }
    program_name = get_program_name(argv[0]);
    install_handlers(program_name);

    enable_logging();
    set_log_level_from_env();

    return run(argc, argv);
}
