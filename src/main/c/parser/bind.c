
#include "log.h"

#include "parser/base.h"


static bool is_terminal(Parser *self)
{
    return REPETITION < self->kind;
}

static bool is_nonterminal(Parser *self)
{
    return !is_terminal(self);
}

MaybeSyntaxNode bind(Parser *self, MaybeSyntaxNode node, Input *input)
{
    static size_t padding = 0;
    if(is_nothing(node))
    {
        return node;    
    }
    log_trace("parser", "%*sentering %s", (2 * padding++), "", parser_repr(self));
    MaybeSyntaxNode result = self->vtable.delegate(self, node, input);
    log_trace("parser",
        "%*sleaving %s, %s", (2 * --padding), "",
        is_nonterminal(self) ? parser_repr(self) : parser_name(self),
        is_nothing(result) ? "failure" : "success");
    return result;
}
