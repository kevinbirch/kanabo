#include <string.h>

#include "panic.h"
#include "evaluator/predicate.h"
#include "evaluator/step.h"
#include "evaluator/test.h"

static bool evaluate_step(Step* each, void *context);
static bool evaluate_root_step(Evaluator *context);
static bool evaluate_single_step(Evaluator *context);
static bool evaluate_recursive_step(Evaluator *context);

Maybe(Nodelist) evaluate_steps(const DocumentSet *model, const JsonPath *path)
{
    evaluator_debug("beginning evaluation of %d steps", vector_length(path->steps));

    Evaluator evaluator;
    memset(&evaluator, 0, sizeof(Evaluator));

    evaluator.results = make_nodelist();
    if(NULL == evaluator.results)
    {
        panic("can't allocate the result nodelist");
    }

    evaluator.model = model;
    evaluator.path = path;

    nodelist_add(evaluator.results, document_set_get(model, 0));

    if(!path_iterate(path, evaluate_step, &evaluator))
    {
        evaluator_debug("aborted, step: %d, code: %d (%s)", evaluator.current_step, evaluator.code, evaluator_strerror(evaluator.code));
        return fail(Nodelist, evaluator.code);
    }

    evaluator_debug("done, found %d matching nodes", nodelist_length(evaluator.results));

    return just(Nodelist, evaluator.results);
}

static bool evaluate_step(Step* each, void *argument)
{
    Evaluator *evaluator = (Evaluator *)argument;
    evaluator_trace("step: %zu", evaluator->current_step);

    bool result = false;
    switch(each->kind)
    {
        case ROOT:
            result = evaluate_root_step(evaluator);
            break;
        case SINGLE:
            result = evaluate_single_step(evaluator);
            break;
        case RECURSIVE:
            result = evaluate_recursive_step(evaluator);
            break;
    }
    if(result && NULL != current_step(evaluator)->predicate)
    {
        result = evaluate_predicate(evaluator);
    }
    if(result)
    {
        evaluator->current_step++;
    }
    return result;
}

static bool evaluate_root_step(Evaluator *evaluator)
{
    evaluator_trace("evaluating root step");
    Document *doc = nodelist_get(evaluator->results, 0);
    Node *root = document_root(doc);
    evaluator_trace("root test: adding root node (%p) from document (%p)", root, doc);
    nodelist_set(evaluator->results, root, 0);
    return true;
}

static bool evaluate_single_step(Evaluator *evaluator)
{
    evaluate_nodelist("step",
                      test_kind_name(current_step(evaluator)->test.kind),
                      apply_node_test);
}

static bool evaluate_recursive_step(Evaluator *evaluator)
{
    evaluate_nodelist("recursive step",
                      test_kind_name(current_step(evaluator)->test.kind),
                      apply_recursive_node_test);
}
