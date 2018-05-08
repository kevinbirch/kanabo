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

typedef enum node_kind NodeKind;

typedef struct node_s Node;

struct vtable_s
{
    void (*free)(Node *);
    size_t (*size)(const Node *);
    bool (*equals)(const Node *, const Node *);
};

struct node_s
{
    struct
    {
        NodeKind  kind;
        uint8_t  *name;
    } tag;

    const struct vtable_s *vtable;
    struct node_s         *parent;
    uint8_t               *anchor;
};

struct document_s
{
    union
    {
        Node base;
        struct node_s;
    };
    Node *root;
};

typedef struct document_s Document;

enum scalar_kind
{
    SCALAR_STRING,
    SCALAR_INTEGER,
    SCALAR_REAL,
    SCALAR_TIMESTAMP,
    SCALAR_BOOLEAN,
    SCALAR_NULL
};

typedef enum scalar_kind ScalarKind;

struct scalar_s
{
    union
    {
        Node base;
        struct node_s;
    };
    ScalarKind    kind;
    uint8_t      *value;
    size_t        length;
};

typedef struct scalar_s Scalar;

struct sequence_s
{
    union
    {
        Node base;
        struct node_s;
    };
    Vector       *values;
};

typedef struct sequence_s Sequence;

struct mapping_s
{
    union
    {
        Node base;
        struct node_s;
    };
    Hashtable    *values;
};

typedef struct mapping_s Mapping;

struct alias_s
{
    union
    {
        Node base;
        struct node_s;
    };
    struct node_s *target;
};

typedef struct alias_s Alias;

// xxx - create file object

typedef Vector DocumentModel;

Node *node_narrow(Node *instance, NodeKind kind, const char * restrict file, int line);
const Node *const_node_narrow(const Node *instance, NodeKind kind, const char * restrict file, int line);
#define CHECKED_CAST(OBJ, KIND, TYPE) (TYPE *)node_narrow((OBJ), (KIND), __FILE__, __LINE__)
#define CONST_CHECKED_CAST(OBJ, KIND, TYPE) (const TYPE *)const_node_narrow((OBJ), (KIND), __FILE__, __LINE__)

/*
 * Constructors
 */

Document *make_document_node(void);
Sequence *make_sequence_node(void);
Mapping  *make_mapping_node(void);
Scalar   *make_scalar_node(const uint8_t *value, size_t length, ScalarKind kind);
Alias    *make_alias_node(Node *target);
#define   make_model() make_vector_with_capacity(1)

/*
 * Destructors
 */

void node_free_(Node *value);
#define node_free(object) node_free_(node((object)))
void model_free(DocumentModel *value);

/*
 * Model API
 */

#define model_size vector_length
#define model_document vector_get
Node   *model_document_root(const DocumentModel *model, size_t index);

bool    model_add(DocumentModel *model, Document *doc);

/*
 * Node API
 */

void node_init(Node *self, NodeKind kind, const struct vtable_s *vtable);

const char *node_kind_name_(const Node *value);
#define     node_kind_name(object) node_kind_name_(node((object)))

NodeKind    node_kind_(const Node *value);
#define     node_kind(object) node_kind_(node((object)))
uint8_t    *node_name_(const Node *value);
#define     node_name(object) node_name_(node((object)))
Node       *node_parent_(const Node *value);
#define     node_parent(object) node_parent_(node((object)))
size_t      node_size_(const Node *value);
#define     node_size(object) node_size_(node((object)))

bool        node_equals_(const Node *one, const Node *two);
#define     node_equals(one, two) node_equals_(const_node((one)), const_node((two)))
bool        node_comparitor(const void *one, const void *two);

void        node_set_tag_(Node *target, const uint8_t *value, size_t length);
#define     node_set_tag(object, value, length) node_set_tag_(node((object)), (value), (length))
void        node_set_anchor_(Node *target, const uint8_t *value, size_t length);
#define     node_set_anchor(object, value, length) node_set_anchor_(node((object)), (value), (length))

#define node(obj) ((Node *)(obj))
#define const_node(obj) ((const Node *)(obj))

/*
 * Document API
 */

Node *document_root(const Document *doc);
bool  document_set_root(Document *doc, Node *root);

#define document(obj) (CHECKED_CAST((obj), DOCUMENT, Document))
#define const_document(obj) (CONST_CHECKED_CAST((obj), DOCUMENT, Document))
#define is_document(obj) (DOCUMENT == node_kind(node((obj))))

/*
 * Scalar API
 */

const char *scalar_kind_name(const Scalar *value);

uint8_t    *scalar_value(const Scalar *scalar);
ScalarKind  scalar_kind(const Scalar *scalar);
bool        scalar_boolean_is_true(const Scalar *scalar);
bool        scalar_boolean_is_false(const Scalar *scalar);

#define scalar(obj) (CHECKED_CAST((obj), SCALAR, Scalar))
#define const_scalar(obj) (CONST_CHECKED_CAST((obj), SCALAR, Scalar))

#define is_scalar(obj) (SCALAR == node_kind(node((obj))))
#define is_string(obj) (is_scalar((obj)) && SCALAR_STRING == scalar_kind(scalar((obj))))
#define is_integer(obj) (is_scalar((obj)) && SCALAR_INTEGER == scalar_kind(scalar((obj))))
#define is_real(obj) (is_scalar((obj)) && SCALAR_REAL == scalar_kind(scalar((obj))))
#define is_number(obj) (is_integer((obj)) || is_real((obj)))
#define is_timestamp(obj) (is_scalar((obj)) && SCALAR_TIMESTAMP == scalar_kind(scalar((obj))))
#define is_boolean(obj) (is_scalar((obj)) && SCALAR_BOOLEAN == scalar_kind(scalar((obj))))
#define is_null(obj) (is_scalar((obj)) && SCALAR_NULL == scalar_kind(scalar((obj))))

/*
 * Sequence API
 */

Node *sequence_get(const Sequence *seq, size_t index);
bool  sequence_add(Sequence *seq, Node *item);

typedef bool (*sequence_iterator)(Node *each, void *context);
bool sequence_iterate(const Sequence *seq, sequence_iterator iterator, void *context);

#define sequence(obj) (CHECKED_CAST((obj), SEQUENCE, Sequence))
#define const_sequence(obj) (CONST_CHECKED_CAST((obj), SEQUENCE, Sequence))
#define is_sequence(obj) (SEQUENCE == node_kind(node((obj))))

/*
 * Mapping API
 */

Node *mapping_get(const Mapping *map, uint8_t *key, size_t length);
bool  mapping_contains(const Mapping *map, uint8_t *scalar, size_t length);
bool  mapping_put(Mapping *map, uint8_t *key, size_t length, Node *value);

typedef bool (*mapping_iterator)(Node *key, Node *value, void *context);
bool mapping_iterate(const Mapping *map, mapping_iterator iterator, void *context);

#define mapping(obj) (CHECKED_CAST((obj), MAPPING, Mapping))
#define const_mapping(obj) (CONST_CHECKED_CAST((obj), MAPPING, Mapping))
#define is_mapping(obj) (MAPPING == node_kind(node((obj))))

/*
 * Alias API
 */

Node *alias_target(const Alias *alias);

#define alias(obj) (CHECKED_CAST((obj), ALIAS, Alias))
#define const_alias(obj) (CONST_CHECKED_CAST((obj), ALIAS, Alias))
#define is_alias(obj) (ALIAS == node_kind(node((obj))))
