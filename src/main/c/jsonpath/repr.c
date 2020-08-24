#include <inttypes.h>

#include "jsonpath.h"

static void repr_path(const JsonPath *path, MutableString **buf);

static void repr_predicate(const Predicate *predicate, MutableString **buf)
{
    if(NULL == predicate)
    {
        return;
    }

    mstring_append(buf, '[');
    switch(predicate->kind)
    {
        case WILDCARD:
            mstring_append(buf, '*');
            break;
        case SUBSCRIPT:
        {
            mformat(buf, "%"PRId64, subscript_predicate_index(predicate));
        }
        break;
        case SLICE:
        {
            if(slice_predicate_has_from(predicate))
            {
                mformat(buf, "%"PRId64, slice_predicate_from(predicate));
            }
            mstring_append(buf, ':');
            if(slice_predicate_has_to(predicate))
            {
                mformat(buf, "%"PRId64, slice_predicate_to(predicate));
            }
            if(slice_predicate_has_step(predicate))
            {
                mstring_append(buf, ':');
                mformat(buf, "%"PRId64, slice_predicate_step(predicate));
            }
        }
        break;
        case JOIN:
        {
            repr_path(join_predicate_left(predicate), buf);
            repr_path(join_predicate_right(predicate), buf);
        }
            break;
    }
    mstring_append(buf, ']');
}

static void repr_test(const Step *step, MutableString **buf)
{
    switch(test_kind(step))
    {
        case WILDCARD_TEST:
            mstring_append(buf, '*');
            break;
        case NAME_TEST:
            mstring_append(buf, name_test_step_name(step));
            break;
        case TYPE_TEST:
            switch(type_test_step_kind(step))
            {
                case OBJECT_TEST:
                    mstring_append(buf, "object()");
                    break;
                case ARRAY_TEST:
                    mstring_append(buf, "array()");
                    break;
                case STRING_TEST:
                    mstring_append(buf, "string()");
                    break;
                case NUMBER_TEST:
                    mstring_append(buf, "number()");
                    break;
                case BOOLEAN_TEST:
                    mstring_append(buf, "boolean()");
                    break;
                case NULL_TEST:
                    mstring_append(buf, "null()");
                    break;                
            }
            break;
    }
}

static void repr_step(const Step *step, PathKind kind, MutableString **buf)
{
    if(ROOT == step->kind)
    {
        switch(kind)
        {
            case ABSOLUTE_PATH:
                mstring_append(buf, '$');
                break;
            case RELATIVE_PATH:
                mstring_append(buf, '@');
                break;
        }
        goto predicate;
    }

    repr_test(step, buf);

  predicate:
    repr_predicate(step->predicate, buf);
}

static void repr_path(const JsonPath *path, MutableString **buf)
{
    for(size_t i = 0; i < vector_length(path->steps); i++)
    {
        Step *step = vector_get(path->steps, i);
        if(0 != i)
        {
            if(SINGLE == step->kind)
            {
                mstring_append(buf, '.');
            }
            else if(RECURSIVE == step->kind)
            {
                mstring_append(buf, "..");
            }
        }

        repr_step(step, path->kind, buf);
    }
}

String *path_repr(const JsonPath *path)
{
    MutableString *buf = make_mstring(1);
    repr_path(path, &buf);

    String *result = mstring_as_string(buf);
    dispose_mstring(buf);

    return result;
}
