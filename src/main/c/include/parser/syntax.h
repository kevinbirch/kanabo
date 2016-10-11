
#pragma once

#include <stdint.h>

#include "str.h"

#include "parser/location.h"

enum cst_node_tag
{
    CST_ROOT = 0,
    CST_RULE,
    CST_COLLECTION,
    CST_LITERAL,
    CST_NUMBER,
    CST_INTEGER,
    CST_TERM
};

typedef enum cst_node_tag ConcreteSyntaxNodeType;

typedef struct syntax_node_s SyntaxNode;

typedef void (*SyntaxNodeVisitor)(SyntaxNode *node, void *context);

SyntaxNode *make_syntax_node(uint_fast16_t type, String *value, SourceLocation loc);

void dispose_syntax_node(SyntaxNode *self);

uint_fast16_t  syntax_node_type(SyntaxNode *self);
String        *syntax_node_value(SyntaxNode *self);

void syntax_node_add_child(SyntaxNode *self, SyntaxNode *child);
void syntax_node_visit_pre_order(SyntaxNode *self, SyntaxNodeVisitor visitor, void *parameter);
