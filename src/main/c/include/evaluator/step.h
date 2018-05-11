#pragma once

#include "evaluator/evaluate.h"

EvaluatorErrorCode evaluate_steps(const DocumentModel *model, const JsonPath *path, Nodelist **list);
