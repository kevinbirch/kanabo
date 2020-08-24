#pragma once

#include "document.h"

Node *sequence_builder(Node *one, ...);
#define seq(one, ...) sequence_builder(one, __VA_ARGS__, NULL)
Node *mapping_builder(const char *key, Node *value, ...);
#define map(key, value, ...) mapping_builder(key, value, __VA_ARGS__, NULL)

Node *string(const char *value);

Node *integer(const char *value);
Node *real(const char *value);
Node *timestamp(const char *value);
Node *boolean(bool value);
Node *null(void);
