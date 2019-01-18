#include <string.h>

#include "panic.h"
#include "evaluator/predicate.h"
#include "evaluator/step.h"
#include "evaluator/test.h"

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

static bool evaluate_step(Step* each, void *argument)
{
    Evaluator *self = (Evaluator *)argument;
    evaluator_trace("step: %zu", self->current_step);

    bool result = false;
    switch(each->kind)
    {
        case ROOT:
            result = evaluate_root_step(self);
            break;
        case SINGLE:
            result = evaluate_single_step(self);
            break;
        case RECURSIVE:
            result = evaluate_recursive_step(self);
            break;
    }

    if(result && NULL != current_step(self)->predicate)
    {
        result = evaluate_predicate(self);
    }

    if(result)
    {
        self->current_step++;
    }

    return result;
}

Maybe(Nodelist) evaluate_steps(const DocumentSet *model, const JsonPath *path)
{
    evaluator_debug("beginning evaluation of %zu steps", vector_length(path->steps));

    Evaluator self;
    memset(&self, 0, sizeof(Evaluator));

    self.results = make_nodelist();
    if(NULL == self.results)
    {
        panic("can't allocate the result nodelist");
    }

    self.model = model;
    self.path = path;

    nodelist_add(self.results, document_set_get(model, 0));

    if(!path_iterate(path, evaluate_step, &self))
    {
        evaluator_debug("aborted, step: %zu, code: %d (%s)", self.current_step, self.code, evaluator_strerror(self.code));
        return fail(Nodelist, self.code);
    }

    evaluator_debug("done, found %zu matching nodes", nodelist_length(self.results));

    return just(Nodelist, self.results);
}
