#pragma once

#include "evaluator.h"
#include "evaluator/debug.h"

struct evaluator_context_s
{
    const DocumentSet  *model;
    const JsonPath     *path;
    size_t              current_step;
    Nodelist           *results;
    EvaluatorErrorCode  code;
};

typedef struct evaluator_context_s Evaluator;

#define evaluate_nodelist(NAME, TEST, FUNCTION)                         \
    evaluator_tracef("evaluating %s across %zu nodes", (NAME), nodelist_length(evaluator->results)); \
    Nodelist *result = nodelist_map(evaluator->results, (FUNCTION), evaluator); \
    evaluator_tracef("%s: %s", (TEST), NULL == result ? "failed" : "completed"); \
    evaluator_tracef("%s: added %zu nodes", (NAME), nodelist_length(result)); \
    return NULL == result ? false : (dispose_nodelist(evaluator->results), evaluator->results = result, true);

#define current_step(CTX) ((Step *)vector_get((CTX)->path->steps, (CTX)->current_step))

bool add_to_nodelist_sequence_iterator(Node *each, void *context);
