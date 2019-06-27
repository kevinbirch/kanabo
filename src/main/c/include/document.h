#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "hashtable.h"
#include "position.h"
#include "str.h"
#include "vector.h"

#define INDENT 4

struct document_set_s
{
    String *input_name;
    Vector *values;
};

typedef struct document_set_s DocumentSet;

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
    void    (*free)   (Node *);
    size_t  (*size)   (const Node *);
    bool    (*equals) (const Node *, const Node *);
    String *(*repr)   (const Node *);
    void    (*dump)   (const Node *, bool);
};

struct node_s
{
    struct
    {
        NodeKind  kind;
        String   *name;
    } tag;

    struct node_s         *parent;
    Position               position;
    String                *anchor;
    const struct vtable_s *vtable;
    size_t                 depth;
};

struct document_s
{
    union
    {
        Node   base;
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
        Node   base;
        struct node_s;
    };
    ScalarKind  kind;
    String     *value;

    union
    {
        int64_t integer;
        double  real;
        bool    boolean;
    };
};

typedef struct scalar_s Scalar;

struct sequence_s
{
    union
    {
        Node   base;
        struct node_s;
    };
    Vector *values;
};

typedef struct sequence_s Sequence;

struct mapping_s
{
    union
    {
        Node   base;
        struct node_s;
    };
    Hashtable *values;
};

typedef struct mapping_s Mapping;

struct alias_s
{
    union
    {
        Node   base;
        struct node_s;
    };
    struct node_s *target;
};

typedef struct alias_s Alias;

/*
 * Constructors
 */

DocumentSet *make_document_set(void);
Document    *make_document_node(void);
Sequence    *make_sequence_node(void);
Mapping     *make_mapping_node(void);
Scalar      *make_scalar_node(String *value, ScalarKind kind);
Alias       *make_alias_node(Node *target);

/*
 * Destructors
 */

void         dispose_document_set(DocumentSet *value);
void         (dispose_node)(Node *value);
#define      dispose_node(SELF) dispose_node(node((SELF)))

/*
 * Document Set API
 */

#define      document_set_size(SELF) (NULL == (SELF) ? (size_t)0 : vector_length((SELF)->values))
#define      document_set_get(SELF, INDEX) (NULL == (SELF) ? NULL : vector_get((SELF)->values, (INDEX)))
Node        *document_set_get_root(const DocumentSet *model, size_t index);
#define      document_set_add(SELF, DOC) if(NULL != (SELF)) {vector_add((SELF)->values, (DOC));}
void         document_set_dump(const DocumentSet *model);

/*
 * Node API
 */

void         node_init(Node *self, NodeKind kind, const struct vtable_s *vtable);

Node        *node_narrow(Node *instance, NodeKind kind, const char * restrict file, int line);
const Node  *const_node_narrow(const Node *instance, NodeKind kind, const char * restrict file, int line);
#define      CHECKED_CAST(OBJ, KIND, TYPE) (TYPE *)node_narrow((Node *)(OBJ), (KIND), __FILE__, __LINE__)
#define      CONST_CHECKED_CAST(OBJ, KIND, TYPE) (const TYPE *)const_node_narrow((const Node *)(OBJ), (KIND), __FILE__, __LINE__)

#define      node(SELF) _Generic((SELF),                        \
                                 Node *: (SELF),                \
                                 default: ((Node *)(SELF))      \
                                 )
#define      const_node(SELF) _Generic((SELF),                          \
                                       const Node *: (SELF),            \
                                       default: ((const Node *)(SELF))  \
                                       )

#define      node_position(SELF) node((SELF)).position
const char  *(node_kind_name)(const Node *value);
#define      node_kind_name(SELF) node_kind_name(const_node((SELF)))

#define      node_kind(SELF) const_node((SELF))->tag.kind
#define      node_name(SELF) const_node((SELF))->tag.name
#define      node_parent(SELF) const_node((SELF))->parent
#define      node_size(SELF) const_node((SELF))->vtable->size(const_node(SELF))

bool         (node_equals)(const Node *one, const Node *two);
#define      node_equals(one, two) node_equals(const_node((one)), const_node((two)))
bool         node_comparitor(const void *one, const void *two);

String *     node_repr(const Node *self);
void         node_dump(const Node *self, bool pad);

void         (node_set_tag)(Node *target, String *value);
#define      node_set_tag(SELF, VALUE) node_set_tag(node((SELF)), (VALUE))
void         (node_set_anchor)(Node *target, String *value);
#define      node_set_anchor(SELF, VALUE) node_set_anchor(node((SELF)), (VALUE))

/*
 * Document API
 */

#define      document_root(SELF) document((SELF))->root
void         document_set_root(Document *doc, Node *root);
void         document_track_anchor(Document *self, uint8_t *value, Node *target);
Node        *document_resolve_anchor(Document *self, uint8_t *value);

#define      document(SELF) _Generic((SELF),\
                                     Document *: (SELF),                \
                                     default: (CHECKED_CAST((SELF), DOCUMENT, Document)) \
                                     )
#define      const_document(SELF) _Generic((SELF),\
                                           const Document *: (SELF),    \
                                           Document *: (const Document *)(SELF), \
                                           default: (CONST_CHECKED_CAST((SELF), DOCUMENT, Document)) \
                                           )
#define      is_document(SELF) (DOCUMENT == node_kind((SELF)))

/*
 * Scalar API
 */

const char  *scalar_kind_name(const Scalar *value);

#define      scalar_value(SELF) scalar((SELF))->value
#define      scalar_kind(SELF) scalar((SELF))->kind
#define      scalar_boolean_is_true(SELF) (0 == memcmp("true", strdta(scalar((SELF))->value), 4))
#define      scalar_boolean_is_false(SELF) (0 == memcmp("false", strdta(scalar((SELF))->value), 5))

#define      scalar(SELF) _Generic((SELF),                              \
                                   Scalar *: (SELF),                    \
                                   default: CHECKED_CAST((SELF), SCALAR, Scalar) \
                                   )
#define      const_scalar(SELF) _Generic((SELF),                        \
                                         const Scalar *: (SELF),        \
                                         Scalar *: (const Scalar *)(SELF), \
                                         default: CONST_CHECKED_CAST((SELF), SCALAR, Scalar) \
                                         )

#define      is_scalar(SELF) (SCALAR == node_kind((SELF)))
#define      is_string(SELF) (is_scalar((SELF)) && SCALAR_STRING == scalar_kind(scalar((SELF))))
#define      is_integer(SELF) (is_scalar((SELF)) && SCALAR_INTEGER == scalar_kind(scalar((SELF))))
#define      is_real(SELF) (is_scalar((SELF)) && SCALAR_REAL == scalar_kind(scalar((SELF))))
#define      is_number(SELF) (is_integer((SELF)) || is_real((SELF)))
#define      is_timestamp(SELF) (is_scalar((SELF)) && SCALAR_TIMESTAMP == scalar_kind(scalar((SELF))))
#define      is_boolean(SELF) (is_scalar((SELF)) && SCALAR_BOOLEAN == scalar_kind(scalar((SELF))))
#define      is_null(SELF) (is_scalar((SELF)) && SCALAR_NULL == scalar_kind(scalar((SELF))))

/*
 * Sequence API
 */

#define      sequence_get(SELF, INDEX) vector_get(const_sequence((SELF))->values, (INDEX))
void         sequence_add(Sequence *seq, Node *item);
#define      sequence_is_empty(SELF) vector_is_empty(const_sequence((SELF))->values)

typedef bool (*sequence_iterator)(Node *each, void *context);
bool         sequence_iterate(const Sequence *seq, sequence_iterator iterator, void *context);

#define      sequence(SELF) _Generic((SELF),\
                                     Sequence *: (SELF),                \
                                     default: CHECKED_CAST((SELF), SEQUENCE, Sequence) \
                                     )
#define      const_sequence(SELF) _Generic((SELF),\
                                           const Sequence *: (SELF),    \
                                           Sequence *: (const Sequence *)(SELF), \
                                           default: (CONST_CHECKED_CAST((SELF), SEQUENCE, Sequence)) \
                                           )
#define      is_sequence(SELF) (SEQUENCE == node_kind((SELF)))

/*
 * Mapping API
 */

Node        *mapping_lookup(const Mapping *map, String *key);
Node        *mapping_get(const Mapping *map, Scalar *key);
bool         mapping_contains(const Mapping *map, String *key);
bool         mapping_put(Mapping *self, Scalar *key, Node *value);
#define      mapping_is_empty(SELF) hashtable_is_empty(const_mapping((SELF))->values)

typedef bool (*mapping_iterator)(String *key, Node *value, void *context);
bool         mapping_iterate(const Mapping *map, mapping_iterator iterator, void *context);

#define      mapping(SELF) _Generic((SELF),\
                                    Mapping *: (SELF),                  \
                                    default: CHECKED_CAST((SELF), MAPPING, Mapping) \
                                    )
#define      const_mapping(SELF) _Generic((SELF),\
                                          const Mapping *: (SELF),      \
                                          Mapping *: (const Mapping *)(SELF), \
                                          default: CONST_CHECKED_CAST((SELF), MAPPING, Mapping) \
                                          )
#define      is_mapping(SELF) (MAPPING == node_kind((SELF)))

/*
 * Alias API
 */

#define      alias_target(SELF) alias((SELF))->target

#define      alias(SELF) _Generic((SELF),\
                                  Alias *: (SELF),                      \
                                  default: (CHECKED_CAST((SELF), ALIAS, Alias)) \
                                  )
#define      const_alias(SELF) _Generic((SELF),\
                                        const Alias *: (SELF),          \
                                        Alias *: (const Alias *)(SELF), \
                                        default: (CONST_CHECKED_CAST((SELF), ALIAS, Alias)) \
                                        )
#define      is_alias(SELF) (ALIAS == node_kind((SELF)))
