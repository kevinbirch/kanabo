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

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/errno.h>
#include <string.h>

#include "warranty.h"
#include "help.h"
#include "options.h"
#include "loader.h"
#include "jsonpath.h"
#include "evaluator.h"
#include "emit.h"
#include "log.h"
#include "version.h"
#include "linenoise.h"

static int dispatch(enum command cmd, const struct settings *settings);

static int interactive_mode(const struct settings *settings);
static int expression_mode(const struct settings *settings);
static int apply_expression(const struct settings *settings, document_model *model, const char *expression);

static document_model *load_model(const struct settings *settings);
static jsonpath *parse_expression(const struct settings *settings, const char *expression);
static nodelist *evaluate_expression(const struct settings *settings, const document_model *model, const jsonpath *path);

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

    return dispatch(cmd, &settings);
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

static int apply_expression(const struct settings *settings, document_model *model, const char *expression)
{
    jsonpath *path = parse_expression(settings, expression);
    if(NULL == path)
    {
        return EXIT_FAILURE;
    }

    nodelist *list = evaluate_expression(settings, model, path);
    if(NULL == list)
    {
        jsonpath_free(path);
        return EXIT_FAILURE;
    }

    emit_function emit = get_emitter(settings);
    if(NULL == emit)
    {
        jsonpath_free(path);
        nodelist_free(list);
        return EXIT_FAILURE;
    }

    emit(list, settings);

    jsonpath_free(path);
    nodelist_free(list);

    return EXIT_SUCCESS;
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

static jsonpath *parse_expression(const struct settings *settings, const char *expression)
{
    log_trace("kanabo", "parsing expression");
    MaybeJsonpath result = parse((uint8_t *)expression, strlen(expression));    
    if(ERROR == result.tag)
    {
        error("while parsing the jsonpath expression", result.error.message, settings);
        maybe_jsonpath_free(result);
        return NULL;
    }

    return result.path;
}

static nodelist *evaluate_expression(const struct settings *settings, const document_model *model, const jsonpath *path)
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
