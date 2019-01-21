#pragma once

#include "log.h"

#define component_name "loader"

#define loader_info(FORMAT, ...)  log_info(component_name, FORMAT, ##__VA_ARGS__)
#define loader_error(FORMAT, ...)  log_error(component_name, FORMAT, ##__VA_ARGS__)
#define loader_debug(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define loader_trace(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(LVL_TRACE, component_name, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)