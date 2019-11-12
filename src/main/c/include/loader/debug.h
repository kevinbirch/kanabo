#pragma once

#include "log.h"

#define component_name "loader"

#define loader_debug(MSG) log_debug(component_name, MSG)
#define loader_debugf(FORMAT, ...) log_debug(component_name, FORMAT, ##__VA_ARGS__)
#define loader_trace(MSG) log_trace(component_name, MSG)
#define loader_tracef(FORMAT, ...) log_trace(component_name, FORMAT, ##__VA_ARGS__)

#define trace_string(FORMAT, VALUE, LENGTH, ...) log_string(LVL_TRACE, component_name, FORMAT, VALUE, LENGTH, ##__VA_ARGS__)
