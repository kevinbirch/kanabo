#pragma once

#include "loader.h"

void add_loader_error(Vector *errors, Position position, LoaderErrorCode code);
void add_loader_error_with_extra(Vector *errors, Position position, LoaderErrorCode code, String *extra);
