#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hashtable.h"
#include "vector.h"

enum node_kind 
{
    DOCUMENT,
    SCALAR,
    SEQUENCE,
    MAPPING,
    ALIAS
};

enum scalar_kind
{
    SCALAR_STRING,
    SCALAR_INTEGER,
    SCALAR_REAL,
    SCALAR_TIMESTAMP,
    SCALAR_BOOLEAN,
    SCALAR_NULL
};

struct node
{
    struct node *parent;
    struct 
    {
        enum node_kind  kind;
        uint8_t        *name;
    } tag;

    uint8_t *anchor;
    struct
    {
        size_t size;
        union
        {
            struct
            {
                enum scalar_kind kind;
                uint8_t         *value;
            } scalar;
        
            Vector      *sequence;
            Hashtable   *mapping;
            struct node *target;
        };
    } content;
};

typedef struct node node;

struct model
{
    Vector *documents;
};

typedef struct model document_model;

/*
 * Constructors
 */

node *make_document_node(void);
node *make_sequence_node(void);
node *make_mapping_node(void);
node *make_scalar_node(const uint8_t *value, size_t length, enum scalar_kind kind);
node *make_alias_node(node *target);
document_model *make_model(void);

/*
 * Destructors
 */

void node_free(node *value);
void model_free(document_model *model);

/*
 * Model API
 */

node   *model_document(const document_model *model, size_t index);
node   *model_document_root(const document_model *model, size_t index);
size_t  model_document_count(const document_model *model);

bool model_add(document_model *model, node *document);

/*
 * Node API
 */

enum node_kind  node_kind(const node *value);
uint8_t        *node_name(const node *value);
size_t          node_size(const node *value);
node           *node_parent(const node *value);

bool node_equals(const node *one, const node *two);

void node_set_tag(node *target, const uint8_t *value, size_t length);
void node_set_anchor(node *target, const uint8_t *value, size_t length);

/*
 * Document API
 */

node *document_root(const node *document);
bool  document_set_root(node *document, node *root);

/*
 * Scalar API
 */

uint8_t *scalar_value(const node *scalar);
enum scalar_kind scalar_kind(const node *scalar);
bool scalar_boolean_is_true(const node *scalar);
bool scalar_boolean_is_false(const node *scalar);

/*
 * Sequence API
 */

node *sequence_get(const node *sequence, int64_t index);
bool  sequence_add(node *sequence, node *item);

typedef bool (*sequence_iterator)(node *each, void *context);
bool sequence_iterate(const node *sequence, sequence_iterator iterator, void *context);

/*
 * Mapping API
 */

node *mapping_get(const node *mapping, uint8_t *key, size_t length);
bool  mapping_contains(const node *mapping, uint8_t *scalar, size_t length);
bool  mapping_put(node *mapping, uint8_t *key, size_t length, node *value);

typedef bool (*mapping_iterator)(node *key, node *value, void *context);
bool mapping_iterate(const node *mapping, mapping_iterator iterator, void *context);

/*
 * Alias API
 */

node *alias_target(const node *alias);

