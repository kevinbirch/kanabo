
#include "vector.h"

#include "parser/syntax.h"


struct syntax_node_s
{
    uint_fast16_t type;

    SourceLocation  location;
    String   *value;
    Vector   *children;
};


SyntaxNode *make_syntax_node(uint_fast16_t type, String *value, SourceLocation loc)
{
    SyntaxNode *self = calloc(1, sizeof(SyntaxNode));

    if(NULL != self)
    {
        self->type = type;
        self->location = loc;
        self->value = value;
    }
    return self;
}

static void syntax_node_destructor(void *value)
{
    SyntaxNode *self = (SyntaxNode *)value;
    dispose_syntax_node(self);
}

void dispose_syntax_node(SyntaxNode *self)
{
    if(NULL == self)
    {
        return;
    }

    if(NULL != self->children)
    {
        vector_destroy(self->children, syntax_node_destructor);
    }
    if(NULL != self->value)
    {
        string_free(self->value);
    }
    free(self);
}

uint_fast16_t syntax_node_type(SyntaxNode *self)
{
    return self->type;
}

inline String *syntax_node_value(SyntaxNode *self)
{
    return self->value;
}

void syntax_node_add_child(SyntaxNode *self, SyntaxNode *child)
{
    if(NULL == self || NULL == child)
    {
        return;
    }

    if(NULL == self->children)
    {
        self->children = make_vector();
        if(NULL == self->children)
        {
            return;
        }
    }

    vector_add(self->children, child);
}

struct iteration_context_s
{
    SyntaxNodeVisitor visitor;
    void *parameter;
};

static bool visitor_adapter(void *each, void *parameter)
{
    SyntaxNode *node = (SyntaxNode *)each;
    struct iteration_context_s *context = (struct iteration_context_s *)parameter;
    syntax_node_visit_pre_order(node, context->visitor, context->parameter);

    return true;
}

void syntax_node_visit_pre_order(SyntaxNode *self, SyntaxNodeVisitor visitor, void *parameter)
{
    if(NULL == self || NULL == visitor)
    {
        return;
    }

    visitor(self, parameter);
    if(NULL != self->children)
    {
        struct iteration_context_s context = {visitor, parameter};
        vector_iterate(self->children, visitor_adapter, &context);
    }
}
