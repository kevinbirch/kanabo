#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/errno.h>
#include <string.h>

#include "linenoise.h"

#include "warranty.h"
#include "help.h"
#include "options.h"
#include "loader.h"
#include "parser.h"
#include "evaluator.h"
#include "emitter.h"
#include "log.h"
#include "version.h"

static int dispatch(enum command cmd, const struct settings *settings);

static int interactive_mode(const struct settings *settings);
static int expression_mode(const struct settings *settings);
static int apply_expression(const struct settings *settings, document_model *model, const char *expression);

static document_model *load_model(const struct settings *settings);
static nodelist *evaluate_expression(const struct settings *settings, const document_model *model, const JsonPath *path);

static FILE *open_input(const struct settings *settings);
static void close_input(const struct settings *settings, FILE *input);

static void error(const char *prelude, const char *message, const struct settings *settings);

static emit_function get_emitter(const struct settings *settings);

int main(const int argc, char * const *argv)
{
    if(NULL == argv || NULL == argv[0])
    {
        fprintf(stderr, "error: whoa! something is wrong, there are no program arguments.\n");
        return EXIT_FAILURE;
    }

    enable_logging();
    set_log_level_from_env();

    struct settings settings;
    memset(&settings, 0, sizeof(settings));
    enum command cmd = process_options(argc, argv, &settings);

    int result = dispatch(cmd, &settings);

    return result;
}

static int dispatch(enum command cmd, const struct settings *settings)
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
            result = interactive_mode(settings);
            break;
        case EXPRESSION_MODE:
            result = expression_mode(settings);
            break;
    }

    return result;
}

static int interactive_mode(const struct settings *settings)
{
    document_model *model = load_model(settings);
    if(NULL == model)
    {
        return EXIT_FAILURE;
    }
    if(NULL == model_document(model, 0))
    {
        error("", "No document data was loaded", settings);
        return EXIT_FAILURE;
    }
    char *prompt = NULL;
    
    if(isatty(fileno(stdin)))
    {
        prompt = ">> ";
        fprintf(stdout, "kanabo %s (built: %s)\n", VERSION, BUILD_DATE);
        fprintf(stdout, "[%s] on %s\n", BUILD_COMPILER, BUILD_HOSTNAME);
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
        if(apply_expression(settings, model, input))
        {
            free(input);
            return EXIT_FAILURE;
        }
        if(!isatty(fileno(stdin)))
        {
            fprintf(stdout, "EOD\n");
            fflush(stdout);
        }
        free(input);
    }
    model_free(model);

    return EXIT_SUCCESS;
}

static int expression_mode(const struct settings *settings)
{
    log_debug("kanabo", "evaluating expression: \"%s\"", settings->expression);
    document_model *model = load_model(settings);
    if(NULL == model)
    {
        return EXIT_FAILURE;
    }
    
    int result = apply_expression(settings, model, settings->expression);
    model_free(model);

    return result;
}

static inline bool error_printer(void *each, void *context)
{
    ParserError *error = (ParserError *)each;
    const char *message = parser_strerror(error->code);
    fprintf(stderr, "expression:%zu error: %s\n", error->position.index + 1, message);

    return true;
}

static int apply_expression(const struct settings *settings, document_model *model, const char *expression)
{
    int result = EXIT_SUCCESS;

    Maybe(JsonPath) maybe = parse(expression);
    if(is_nothing(maybe))
    {
        if(isatty(fileno(stdin)))
        {
            vector_iterate(from_nothing(maybe), error_printer, NULL);
        }
        result = EXIT_FAILURE;
        goto end;
    }

    nodelist *list = evaluate_expression(settings, model, &from_just(maybe));
    if(NULL == list)
    {
        result = EXIT_FAILURE;
        goto cleanup;
    }

    emit_function emit = get_emitter(settings);
    if(NULL == emit)
    {
        error("", "internal error: no emitter configured!", settings);
        result = EXIT_FAILURE;
        goto cleanup;
    }

    emit(list, settings);

  cleanup:
    nodelist_free(list);
    dispose_path(from_just(maybe));

  end:
    return result;
}

static document_model *load_model(const struct settings *settings)
{
    log_trace("kanabo", "loading model");
    FILE *input = open_input(settings);
    if(NULL == input)
    {
        perror(settings->program_name);
        return NULL;
    }
    
    loader_context *loader = make_file_loader(input);
    if(NULL == loader)
    {
        perror(settings->program_name);
        return NULL;
    }
    if(loader_status(loader))
    {
        char *message = loader_status_message(loader);
        error("while loading the data", message, settings);
        free(message);
        close_input(settings, input);
        loader_free(loader);
        return NULL;
    }
    loader_set_dupe_strategy(loader, settings->duplicate_strategy);

    document_model *model = load(loader);    
    if(loader_status(loader))
    {
        char *message = loader_status_message(loader);
        error("while loading the data", message, settings);
        free(message);
        model_free(model);
        model = NULL;
    }

    close_input(settings, input);
    loader_free(loader);
    return model;
}

static nodelist *evaluate_expression(const struct settings *settings, const document_model *model, const JsonPath *path)
{
    log_trace("kanabo", "evaluating expression");
    evaluator_context *evaluator = make_evaluator(model, path);
    if(NULL == evaluator)
    {
        perror(settings->program_name);
        return NULL;
    }
    if(evaluator_status(evaluator))
    {
        const char *message = evaluator_status_message(evaluator);
        error("while evaluating the jsonpath expression", message, settings);
        evaluator_free(evaluator);
        return NULL;
    }

    nodelist *list = evaluate(evaluator);
    if(evaluator_status(evaluator))
    {
        const char *message = evaluator_status_message(evaluator);
        error("while evaluating the jsonpath expression", message, settings);
        nodelist_free(list);
        list = NULL;
    }

    evaluator_free(evaluator);
    return list;
}

static FILE *open_input(const struct settings *settings)
{
    if(NULL == settings->input_file_name)
    {
        log_debug("kanabo", "reading from stdin");
        return stdin;
    }
    else
    {
        log_debug("kanabo", "reading from file: '%s'", settings->input_file_name);
        return fopen(settings->input_file_name, "r");
    }
}

static void close_input(const struct settings *settings, FILE *input)
{
    if(NULL != settings->input_file_name)
    {
        log_trace("kanabo", "closing input file");
        errno = 0;
        if(fclose(input))
        {
            perror(settings->program_name);
        }
    }
}

static void error(const char *prelude, const char *message, const struct settings *settings)
{
    if(!isatty(fileno(stdin)))
    {
        return;
    }
    if(INTERACTIVE_MODE == settings->command)
    {
        fprintf(stderr, "%s\n", message);
    }
    else
    {
        fprintf(stderr, "%s: %s - %s\n", settings->program_name, prelude, message);
    }
}

static emit_function get_emitter(const struct settings *settings)
{
    emit_function result = NULL;
    switch(settings->emit_mode)
    {
        case BASH:
            log_debug("kanabo", "using bash emitter");
            result = emit_bash;
            break;
        case ZSH:
            log_debug("kanabo", "using zsh emitter");
            result = emit_zsh;
            break;
        case JSON:
            log_debug("kanabo", "using json emitter");
            result = emit_json;
            break;
        case YAML:
            log_debug("kanabo", "using yaml emitter");
            result = emit_yaml;
            break;
    }

    return result;
}
