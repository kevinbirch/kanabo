#ifdef __linux__
#define _POSIX_C_SOURCE 200809L  // for fileno, getline
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
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
    "Usage:\n"
    "    kanabo [-o <format>] [-d <strategy>] [ [-q <jsonpath>] (<file> | '-') ]\n"
    "    kanabo [Standalone Options]\n"
    "\n"
    "Options:\n"
    "-q, --query <jsonpath>      Specify a single JSONPath query to execute against the input document and exit.\n"
    "-o, --output <format>       Specify the output format (\"bash\", \"zsh\", \"json\" (default) or \"yaml\").\n"
    "-d, --duplicate <strategy>  Specify how to handle duplicate mapping keys (\"clobber\" (default), \"warn\" or \"fail\").\n"
    "\n"
    "Standalone Options:\n"
    "-v, --version               Print the version information and exit.\n"
    "-w, --no-warranty           Print the no-warranty information and exit.\n"
    "-h, --help                  Print the usage summary and exit.\n";

static const char * const DEFAULT_PROMPT = ">> ";
static const char * const BANNER =
    "kanabo " VERSION " (built: " BUILD_DATE ")\n"
    "[" BUILD_COMPILER "] on " BUILD_HOST_OS "\n"
    "Type \":help\" for instructions\n"
    "\n";
static const char * const INTERACTIVE_HELP =
    "The following commands are available, any other input is treated as a JSONPath expression.\n"
    "\n"
    ":load <path>             Load JSON/YAML data from the file <path>.\n"
    ":output [<format>]       Get/set the output format. (\"bash\", \"zsh\", \"json\" or \"yaml\").\n"
    ":duplicate [<strategy>]  Get/set the strategy to handle duplicate mapping keys (\"clobber\" (default) or \"fail\").\n"
    "\n";

#define is_stdin_filename(NAME) \
    0 == memcmp("-", (NAME), 1)

#define use_stdin(NAME) \
    NULL == (NAME) || is_stdin_filename((NAME))

#define get_input_name(NAME) \
    use_stdin((NAME)) ? "stdin" : (NAME)

static const char *program_name = NULL;

#define kanabo_debug(FORMAT, ...) log_debug(program_name, (FORMAT), ##__VA_ARGS__)
#define kanabo_trace(FORMAT, ...) log_trace(program_name, (FORMAT), ##__VA_ARGS__)

static void vsay(const char *prelude, const char *format, va_list args)
{
    fputs(prelude, stdout);
    vfprintf(stdout, format, args);
    fputc('\n', stdout);
}

__attribute__((__format__ (__printf__, 2, 3)))
static void ok(struct options *options, const char *format, ...)
{
    if(options->mode == EXPRESSION_MODE)
    {
        return;
    }

    va_list rest;
    va_start(rest, format);

    vsay("+OK ", format, rest);

    va_end(rest);
}

__attribute__((__format__ (__printf__, 2, 3)))
static void err(struct options *options, const char *format, ...)
{
    va_list rest;
    va_start(rest, format);

    const char *prelude = options->mode == INTERACTIVE_MODE ? "-ERR " : "";
    vsay(prelude, format, rest);

    va_end(rest);
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
    ParserError *error = (ParserError *)each;
    struct options *options = (struct options *)context;

    if(INTERNAL_ERROR == error->code)
    {
        ParserInternalError *ierror = (ParserInternalError *)error;
        err(options, "%s:%d parser: internal error: %s", ierror->filename, ierror->line, C(ierror->message));
    }
    else
    {
        err(options, "expression:1:%zu %s", error->position.index + 1, parser_strerror(error->code));
    }

    return true;
}

static bool emit(Nodelist *list, struct options *options)
{
    emit_function emitter = get_emitter(options->emit_mode);
    if(!emitter(list))
    {
        // xxx - show actual emitter error here
        err(options, "emitter: internal error: unable to emit results");
        return false;
    }

    return true;
}

static bool apply_expression(const char *expression, DocumentSet *documents, struct options *options)
{
    bool result = false;
    JsonPath *path = NULL;
    Nodelist * list = NULL;

    kanabo_debug("applying expression: \"%s\"", expression);
    Maybe(JsonPath) mp = parse(expression);
    if(is_nothing(mp))
    {
        vector_iterate(from_nothing(mp), parser_error_printer, options);
        parser_dispose_errors(from_nothing(mp));

        goto end;
    }
    path = from_just(mp);

    Maybe(Nodelist) ml = evaluate(documents, path);
    if(is_nothing(ml))
    {
        // xxx - need problem position in json path
        err(options, "expression:1:%zu %s", 0ul, evaluator_strerror(from_nothing(ml)));
        goto cleanup;
    }
    list = from_just(ml);

    if(!emit(list, options))
    {
        goto cleanup;
    }

    result = true;
    size_t length = nodelist_length(list);
    ok(options, "%zu node%s matched", length, 1 == length ? "" : "s");

  cleanup:
    dispose_nodelist(list);
    dispose_path(path);

  end:
    return result;
}

static inline bool loader_error_printer(void *each, void *context)
{
    LoaderError *error = (LoaderError *)each;
    struct options *options = (struct options *)context;

    const char *name = options->input_file_name;
    size_t line = error->position.line + 1;
    size_t offset = error->position.offset + 1;
    const char *message = loader_strerror(error->code);

    if(NULL == error->extra)
    {
        err(options, "%s:%zu:%zu: %s", name, line, offset, message);
    }
    else
    {
        err(options, "%s:%zu:%zu: %s: %s", name, line, offset, message, C(error->extra));
    }

    return true;
}

static DocumentSet *load_input(const char *file_name, struct options *options)
{
    const char *input_name = get_input_name(file_name);
    
    Maybe(DocumentSet) documents;

    if(use_stdin(file_name))
    {
        documents = load_yaml_from_stdin(options->duplicate_strategy);
    }
    else
    {
        Maybe(Input) input = make_input_from_file(file_name);
        if(is_nothing(input))
        {
            const char *problem = input_strerror(input.error.code);
            err(options, "%s: %s: %s", input_name, problem, strerror(input.error.errno_val));

            return NULL;
        }

        documents = load_yaml(from_just(input), options->duplicate_strategy);
        dispose_input(from_just(input));
    }

    if(is_nothing(documents))
    {
        vector_iterate(from_nothing(documents), loader_error_printer, options);
        loader_dispose_errors(from_nothing(documents));

        return NULL;
    }

    ok(options, "loaded \"%s\"", file_name);
    return from_just(documents);
}

static void output_command(const char *argument, struct options *options)
{
    kanabo_debug("processing output command...");
    if(!argument)
    {
        ok(options, "%s", emit_mode_name(options->emit_mode));
        return;
    }

    int32_t mode = parse_emit_mode(argument);
    if(-1 == mode)
    {
        err(options, "unsupported output format: \"%s\"", argument);
        return;
    }

    kanabo_debug("setting value to: %s", argument);
    options->emit_mode = (enum emit_mode)mode;

    ok(options, "%s", emit_mode_name(options->emit_mode));
}

static void duplicate_command(const char *argument, struct options *options)
{
    kanabo_debug("processing duplicate command...");
    if(!argument)
    {
        ok(options, "%s", duplicate_strategy_name(options->duplicate_strategy));
        return;
    }

    int32_t strategy = parse_duplicate_strategy(argument);
    if(-1 == strategy)
    {
        err(options, "unsupported duplicate strategy: \"%s\"", argument);
        return;
    }

    kanabo_debug("setting value to: %s", argument);
    options->duplicate_strategy = (DuplicateKeyStrategy)strategy;

    ok(options, "%s", duplicate_strategy_name(options->duplicate_strategy));
}

static void load_command(const char *argument, struct options *options, DocumentSet **documents)
{
    kanabo_debug("processing load command...");
    if(!argument)
    {
        err(options, "usage: \":load <filename>\"");
        return;
    }

    DocumentSet *new_documents = load_input(argument, options);
    if(NULL != new_documents)
    {
        dispose_document_set(*documents);
        *documents = new_documents;
        options->input_file_name = argument;
    }
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
    size_t clen = strlen(command);
    if(0 == memcmp("?", command, 1) || 0 == memcmp(":help", command, (5 > clen ? clen : 5)))
    {
        fputs(INTERACTIVE_HELP, stdout);
    }
    else if(0 == memcmp(":output", command, (7 > clen ? clen : 7)))
    {
        output_command(get_argument(command), options);
    }
    else if(0 == memcmp(":duplicate", command, (10 > clen ? clen : 10)))
    {
        duplicate_command(get_argument(command), options);
    }
    else if(0 == memcmp(":load", command, (5 > clen ? clen : 5)))
    {
        load_command(get_argument(command), options, documents);
    }
    else
    {
        if(NULL == *documents)
        {
            err(options, "no input loaded, use the \":load\" command");
            return;
        }
        apply_expression(command, *documents, options);
    }
}

static void tty_interactive_mode(struct options *options)
{
    kanabo_debug("entering tty interative mode");

    fputs(BANNER, stdout);
    char *prompt = (char *)DEFAULT_PROMPT;

    DocumentSet *documents = NULL;
    if(options->input_file_name)
    {
        documents = load_input(options->input_file_name, options);
    }

    while(true)
    {
        char *input = linenoise(prompt);
        if(NULL == input)
        {
            break;
        }
        if('\0' == input[0])
        {
            goto reset;
        }

        linenoiseHistoryAdd(input);
        dispatch_interactive_command(input, options, &documents);

      reset:
        fflush(stdout);
        free(input);
    }
    
    dispose_document_set(documents);
}

static void pipe_interactive_mode(struct options *options)
{
    kanabo_debug("entering non-tty interative mode");

    char *input = NULL;
    size_t len = 0;
    ssize_t read;

    DocumentSet *documents = NULL;
    if(options->input_file_name)
    {
        documents = load_input(options->input_file_name, options);
    }

    while((read = getline(&input, &len, stdin)) != -1)
    {
        if(0 == read || '\n' == input[0])
        {
            goto reset;
        }
        input[read - 1] = '\0';  // N.B. - `read` should always be positive here
        dispatch_interactive_command(input, options, &documents);

      reset:
        fflush(stdout);
        free(input);
        input = NULL;
    }

    free(input);
    dispose_document_set(documents);
}

static void interactive_mode(struct options *options)
{
    if(isatty(fileno(stdin)))
    {
        tty_interactive_mode(options);
    }
    else
    {
        pipe_interactive_mode(options);
    }
}

static int expression_mode(struct options *options)
{
    int result = EXIT_FAILURE;

    DocumentSet *documents = load_input(options->input_file_name, options);
    if(NULL == documents)
    {
        goto end;
    }

    if(apply_expression(options->expression, documents, options))
    {
        result = EXIT_SUCCESS;
    }
    dispose_document_set(documents);

  end:
    return result;
}

static int run(const int argc, char * const *argv)
{
    struct options options;
    memset(&options, 0, sizeof(struct options));
    process_options(argc, argv, &options);

    int result = EXIT_SUCCESS;

    switch(options.mode)
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
            interactive_mode(&options);
            break;
        case EXPRESSION_MODE:
            result = expression_mode(&options);
            break;
    }

    return result;
}

static void handle_signal(int sigval)
{
    void *stack[20];
    int depth;

    if(SIGSEGV == sigval || SIGABRT == sigval)
    {
        depth = backtrace(stack, 20);
        fputs("Backtrace follows (most recent first):\n", stderr);
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
        fputs("error: there are no program arguments\n", stderr);
        return EXIT_FAILURE;
    }
    
    program_name = get_program_name(argv[0]);
    install_handlers(program_name);

    enable_logging();
    set_log_level_from_env();

    return run(argc, argv);
}
