
#include <stdio.h>

#include "parser/location.h"
#include "parser/base.h"


struct rule_parser_s
{
    Parser         base;
    const char    *name;
    Parser        *expression;
    tree_rewriter  rewriter;
};

typedef struct rule_parser_s RuleParser;


static void rule_free(Parser *value)
{
    free(value->repr);
    RuleParser *self = (RuleParser *)value;
    parser_free(self->expression);
}

static uint_fast16_t binder(void *a, void **result)
{
    String *rule_name = make_string(self->name);
    SyntaxNode *rule_node = make_syntax_node(CST_RULE, rule_name, location_from_input(input));
    syntax_node_add_child(rule_node, value(expression_node));
    if(NULL != self->rewriter)
    {
        return self->rewriter(just_node(rule_node));
    }
}

static Maybe rule_delegate(Parser *parser, Input *input)
{
    ensure_more_input(input);

    RuleParser *self = (RuleParser *)parser;
    return bind(parser_execute(self->expression, input), binder);
}

Parser *rule_parser(const char *name, Parser *expression, tree_rewriter rewriter)
{
    if(NULL == name)
    {
        return NULL;
    }
    // xxx - find rule parser in cache here
    // xxx - how to auto-initialize the rule cache and hold it elsewhere?
    if(NULL == expression)
    {
        return NULL;
    }
    RuleParser *self = (RuleParser *)calloc(1, sizeof(RuleParser));
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, RULE);
    self->base.vtable.delegate = rule_delegate;
    self->base.vtable.free = rule_free;
    asprintf(&self->base.repr, "rule %s", name);
    self->name = name;
    self->expression = expression;
    self->rewriter = rewriter;

    return (Parser *)self;
}



