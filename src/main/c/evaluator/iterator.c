#include "evaluator/evaluate.h"

bool add_to_nodelist_sequence_iterator(Node *each, void *context)
{
    Nodelist *list = (Nodelist *)context;
    Node *value = each;
    if(is_alias(each))
    {
        value = alias_target(alias(each));
    }
    nodelist_add(list, value);
    return true;
}
