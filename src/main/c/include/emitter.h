#include "emitter/bash.h"
#include "emitter/zsh.h"
#include "emitter/json.h"
#include "emitter/yaml.h"

typedef bool (*emit_function)(const Nodelist *list);
