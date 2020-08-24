#pragma once

#include "log.h"

#define component_name "evaluator"

#define evaluator_debug(MSG) log_debug(component_name, MSG)
#define evaluator_debugf(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define evaluator_trace(MSG) log_trace(component_name, MSG)
#define evaluator_tracef(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(LVL_TRACE, component_name, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)
