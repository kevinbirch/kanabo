#pragma once

#include "evaluator.h"
#include "log.h"

struct evaluator_context
{
    enum evaluator_status_code code;
    size_t                     current_step;
    const DocumentModel       *model;
    const JsonPath            *path;
    nodelist                  *list;
};

typedef struct evaluator_context evaluator_context;

evaluator_status_code evaluate_steps(const DocumentModel *model, const jsonpath *path, nodelist **list);
const char *evaluator_status_message(evaluator_status_code code);

#define component_name "evaluator"

#define evaluator_info(FORMAT, ...)  log_info(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_error(FORMAT, ...)  log_error(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_debug(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_trace(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(LVL_TRACE, component_name, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)
