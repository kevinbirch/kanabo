#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hashtable.h"
#include "position.h"
#include "str.h"
#include "vector.h"

typedef Vector DocumentSet;

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

    Position              position;
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
    Node      *root;
    Hashtable *anchors;
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
    String       *value;
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

/*
 * Constructors
 */

#define      make_document_set() make_vector_with_capacity(1)
Document    *make_document_node(void);
Sequence    *make_sequence_node(void);
Mapping     *make_mapping_node(void);
Scalar      *make_scalar_node(String *value, ScalarKind kind);
Alias       *make_alias_node(Node *target);

/*
 * Destructors
 */

void        dispose_document_set(DocumentSet *value);
void        _dispose_node(Node *value);
#define     dispose_node(SELF) _dispose_node(node((SELF)))

/*
 * Document Set API
 */

#define     document_set_size vector_length
#define     document_set_get vector_get
Node       *document_set_get_root(const DocumentSet *model, size_t index);
bool        document_set_add(DocumentSet *model, Document *doc);

/*
 * Node API
 */

void        node_init(Node *self, NodeKind kind, const struct vtable_s *vtable);

Node       *node_narrow(Node *instance, NodeKind kind, const char * restrict file, int line);
const Node *const_node_narrow(const Node *instance, NodeKind kind, const char * restrict file, int line);
#define     CHECKED_CAST(OBJ, KIND, TYPE) (TYPE *)node_narrow((OBJ), (KIND), __FILE__, __LINE__)
#define     CONST_CHECKED_CAST(OBJ, KIND, TYPE) (const TYPE *)const_node_narrow((OBJ), (KIND), __FILE__, __LINE__)

#define     node(SELF) ((Node *)(SELF))
#define     const_node(SELF) ((const Node *)(SELF))

#define     node_position(SELF) (SELF).position
const char *node_kind_name_(const Node *value);
#define     node_kind_name(SELF) node_kind_name_(node((SELF)))

NodeKind    node_kind_(const Node *value);
#define     node_kind(SELF) node_kind_(node((SELF)))
uint8_t    *node_name_(const Node *value);
#define     node_name(SELF) node_name_(node((SELF)))
Node       *node_parent_(const Node *value);
#define     node_parent(SELF) node_parent_(node((SELF)))
size_t      node_size_(const Node *value);
#define     node_size(SELF) node_size_(node((SELF)))

bool        node_equals_(const Node *one, const Node *two);
#define     node_equals(one, two) node_equals_(const_node((one)), const_node((two)))
bool        node_comparitor(const void *one, const void *two);

void        node_set_tag_(Node *target, const uint8_t *value, size_t length);
#define     node_set_tag(SELF, value, length) node_set_tag_(node((SELF)), (value), (length))
void        node_set_anchor_(Node *target, const uint8_t *value, size_t length);
#define     node_set_anchor(SELF, value, length) node_set_anchor_(node((SELF)), (value), (length))

/*
 * Document API
 */

#define     document_root(SELF) (SELF)->root
void        document_set_root(Document *doc, Node *root);

#define     document(SELF) (CHECKED_CAST((SELF), DOCUMENT, Document))
#define     const_document(SELF) (CONST_CHECKED_CAST((SELF), DOCUMENT, Document))
#define     is_document(SELF) (DOCUMENT == node_kind(node((SELF))))

/*
 * Scalar API
 */

const char *scalar_kind_name(const Scalar *value);

#define     scalar_value(SELF) (SELF)->value
#define     scalar_kind(SELF) (SELF)->kind
#define     scalar_boolean_is_true(SELF) (0 == memcmp("true", strdta((SELF)->value), 4))
#define     scalar_boolean_is_false(SELF) (0 == memcmp("false", strdta((SELF)->value), 5))

#define     scalar(SELF) (CHECKED_CAST((SELF), SCALAR, Scalar))
#define     const_scalar(SELF) (CONST_CHECKED_CAST((SELF), SCALAR, Scalar))

#define     is_scalar(SELF) (SCALAR == node_kind(node((SELF))))
#define     is_string(SELF) (is_scalar((SELF)) && SCALAR_STRING == scalar_kind(scalar((SELF))))
#define     is_integer(SELF) (is_scalar((SELF)) && SCALAR_INTEGER == scalar_kind(scalar((SELF))))
#define     is_real(SELF) (is_scalar((SELF)) && SCALAR_REAL == scalar_kind(scalar((SELF))))
#define     is_number(SELF) (is_integer((SELF)) || is_real((SELF)))
#define     is_timestamp(SELF) (is_scalar((SELF)) && SCALAR_TIMESTAMP == scalar_kind(scalar((SELF))))
#define     is_boolean(SELF) (is_scalar((SELF)) && SCALAR_BOOLEAN == scalar_kind(scalar((SELF))))
#define     is_null(SELF) (is_scalar((SELF)) && SCALAR_NULL == scalar_kind(scalar((SELF))))

/*
 * Sequence API
 */

#define     sequence_get(SELF, INDEX) vector_get((SELF)->values, (INDEX))
bool        sequence_add(Sequence *seq, Node *item);

typedef bool (*sequence_iterator)(Node *each, void *context);
bool        sequence_iterate(const Sequence *seq, sequence_iterator iterator, void *context);

#define     sequence(SELF) (CHECKED_CAST((SELF), SEQUENCE, Sequence))
#define     const_sequence(SELF) (CONST_CHECKED_CAST((SELF), SEQUENCE, Sequence))
#define     is_sequence(SELF) (SEQUENCE == node_kind(node((SELF))))

/*
 * Mapping API
 */

Node       *mapping_get(const Mapping *map, String *key);
bool        mapping_contains(const Mapping *map, String *key);
bool        mapping_put(Mapping *self, String *key, Node *value);

typedef bool (*mapping_iterator)(String *key, Node *value, void *context);
bool        mapping_iterate(const Mapping *map, mapping_iterator iterator, void *context);

#define     mapping(SELF) (CHECKED_CAST((SELF), MAPPING, Mapping))
#define     const_mapping(SELF) (CONST_CHECKED_CAST((SELF), MAPPING, Mapping))
#define     is_mapping(SELF) (MAPPING == node_kind(node((SELF))))

/*
 * Alias API
 */

#define     alias_target(SELF) (SELF)->target

#define     alias(SELF) (CHECKED_CAST((SELF), ALIAS, Alias))
#define     const_alias(SELF) (CONST_CHECKED_CAST((SELF), ALIAS, Alias))
#define     is_alias(SELF) (ALIAS == node_kind(node((SELF))))
