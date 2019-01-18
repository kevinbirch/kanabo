#ifdef __linux__
#define _POSIX_C_SOURCE 200809L  // for fileno, getline
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


#include "emitter.h"
#include "evaluator.h"
#include "linenoise.h"
#include "loader.h"
#include "log.h"
#include "options.h"
#include "parser.h"
#include "version.h"
#include "warranty.h"

static const char * const DEFAULT_PROGRAM_NAME = "kanabo";

static const char * const HELP =
    "usage: kanabo [-o <format>] [-d <strategy>] [ [-q <jsonpath>] (<file> | '-') ]\n"
    "\n"
    "OPTIONS:\n"
    "-q, --query <jsonpath>      Specify a single JSONPath query to execute against the input document and exit.\n"
    "-o, --output <format>       Specify the output format (`bash', `zsh', `json' (default) or `yaml').\n"
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

#define kanabo_debug(FORMAT, ...) log_debug(program_name, (FORMAT), ##__VA_ARGS__)
#define kanabo_trace(FORMAT, ...) log_trace(program_name, (FORMAT), ##__VA_ARGS__)

__attribute__((__format__ (__printf__, 1, 2)))
static void error(const char *format, ...)
{
    va_list rest;
    va_start(rest, format);
    vfprintf(stderr, format, rest);
    va_end(rest);
    fputc('\n', stderr);
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

static inline bool parser_error_printer(void *each, void *context)
{
    ParserError *err = (ParserError *)each;
    if(INTERNAL_ERROR == err->code)
    {
        ParserInternalError *ierr = (ParserInternalError *)err;
        error("%s:%d internal error: %s\n", ierr->filename, ierr->line, ierr->message);
    }
    else
    {
        error("expression:%zu error: %s\n", err->position.index + 1, parser_strerror(err->code));
    }

    return true;
}

static int apply_expression(const char *expression, DocumentSet *documents, enum emit_mode emit_mode)
{
    int result = EXIT_SUCCESS;

    kanabo_debug("applying expression: \"%s\"", expression);
    Maybe(JsonPath) path = parse(expression);
    if(is_nothing(path))
    {
        vector_iterate(from_nothing(path), parser_error_printer, NULL);
        result = EXIT_FAILURE;
        goto end;
    }

    Nodelist *list = maybe(evaluate(documents, from_just(path)), NULL);
    if(NULL == list)
    {
        result = EXIT_FAILURE;
        goto cleanup;
    }

    emit_function emitter = get_emitter(emit_mode);
    if(!emitter(list))
    {
        error("unable to emit results");
        result = EXIT_FAILURE;
    }

  cleanup:
    dispose_nodelist(list);
    dispose_path(from_just(path));

  end:
    return result;
}

static inline bool loader_error_printer(void *each, void *context)
{
    LoaderError *err = (LoaderError *)each;
    const char *input_name = (const char *)context;

    error("%s:%zd:%zd: error: %s", input_name, err->position.line+1, err->position.offset+1, loader_strerror(err->code));

    return true;
}

static Maybe(DocumentSet) load_input(const char *file_name, DuplicateKeyStrategy strategy)
{
    const char *input_name = get_input_name(file_name);
    
    Maybe(DocumentSet) documents;

    if(use_stdin(file_name))
    {
        documents = load_yaml_from_stdin(strategy);
    }
    else
    {
        Maybe(Input) input = make_input_from_file(file_name);
        if(is_nothing(input))
        {
            error("error: %s: %s\n", input_strerror(input.error.code), strerror(input.error.err));
            return nothing(DocumentSet);
        }

        documents = load_yaml(from_just(input), strategy);
        dispose_input(from_just(input));
    }

    if(is_nothing(documents))
    {
        vector_iterate(from_nothing(documents), loader_error_printer, (void *)input_name);
    }

    return documents;
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
    options->duplicate_strategy = (DuplicateKeyStrategy)strategy;
}

static Maybe(DocumentSet) load_command(const char *argument, struct options *options)
{
    kanabo_debug("processing load command...");
    if(!argument)
    {
        kanabo_trace("no command argument, aborting...");
        error(":load command requires an argument");
        return nothing(DocumentSet);
    }

    return load_input(argument, options->duplicate_strategy);
}

static const char *get_argument(const char *command)
{
    char *arg = (char *)command;
    while(!isspace(*arg) && '\0' != *arg)
    {
        arg++;
    }

    if('\0' == *arg)
    {
        return NULL;
    }

    while('\0' != *arg && isspace(*arg))
    {
        arg++;
    }
    if('\0' == *arg)
    {
        return NULL;
    }

    return arg;
}

static void dispatch_interactive_command(const char *command, struct options *options, DocumentSet **documents)
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
        Maybe(DocumentSet) new_documents = load_command(get_argument(command), options);
        if(is_just(new_documents))
        {
            dispose_document_set(*documents);
            *documents = from_just(new_documents);
        }
    }
    else
    {
        if(NULL == *documents)
        {
            error("no input loaded, use the `:load' command");
            return;
        }
        apply_expression(command, *documents, options->emit_mode);
    }
}

static void tty_interctive_mode(struct options *options)
{
    kanabo_debug("entering tty interative mode");

    fwrite(BANNER, strlen(BANNER), 1, stdout);
    char *prompt = (char *)DEFAULT_PROMPT;

    DocumentSet *documents = NULL;
    if(options->input_file_name)
    {
        Maybe(DocumentSet) docs = load_input(options->input_file_name, options->duplicate_strategy);
        if(is_just(docs))
        {
            documents = from_just(docs);
        }
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
        dispatch_interactive_command(input, options, &documents);
        free(input);
    }
    
    dispose_document_set(documents);
}

static void pipe_interactive_mode(struct options *options)
{
    char *input = NULL;
    size_t len = 0;
    ssize_t read;

    DocumentSet *documents = NULL;
    if(options->input_file_name)
    {
        Maybe(DocumentSet) docs = load_input(options->input_file_name, options->duplicate_strategy);
        if(is_just(docs))
        {
            documents = from_just(docs);
        }
    }

    kanabo_debug("entering non-tty interative mode");
    while((read = getline(&input, &len, stdin)) != -1)
    {
        if(0 == read || '\n' == input[0])
        {
            continue;
        }
        input[read - 1] = '\0';  // N.B. `read` should always be positive here
        dispatch_interactive_command(input, options, &documents);
        fputs("EOD\n", stdout);
        fflush(stdout);
    }

    free(input);
    dispose_document_set(documents);
}

static int interactive_mode(struct options *options)
{
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
    Maybe(DocumentSet) documents = load_input(options->input_file_name, options->duplicate_strategy);
    if(is_nothing(documents))
    {
        return EXIT_FAILURE;
    }

    int result = apply_expression(options->expression, from_just(documents), options->emit_mode);
    dispose_document_set(from_just(documents));

    return result;
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
        error("Backtrace follows (most recent first):\n");
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
        fputs("error: something is wrong, there are no program arguments\n", stderr);
        return EXIT_FAILURE;
    }
    
    program_name = get_program_name(argv[0]);
    install_handlers(program_name);

    enable_logging();
    set_log_level_from_env();

    return run(argc, argv);
}
