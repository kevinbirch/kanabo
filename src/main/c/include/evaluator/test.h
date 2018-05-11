#pragma once

#include "evaluator/evaluate.h"

bool apply_node_test(Node *each, void *argument, Nodelist *target);
bool apply_recursive_node_test(Node *each, void *argument, Nodelist *target);
