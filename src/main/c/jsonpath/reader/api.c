
#include "conditions.h"

#include "jsonpath.h"

#include "jsonpath/ast.h"
#include "jsonpath/grammar.h"
#include "jsonpath/model.h"
#include "jsonpath/messages.h"
#include "jsonpath/logging.h"


#define path_error(CODE, POSITION)                                      \
    (MaybeJsonPath){                                                    \
        .tag=NOTHING,                                                   \
            .error.code=(CODE),                                         \
            .error.position=(POSITION),                                 \
            .error.message=status_message((CODE), (POSITION))           \
            }

#define PRECOND_ELSE_NOTHING(CODE, ...)                                 \
    if(is_false(__VA_ARGS__, -1))                                       \
    {                                                                   \
        return path_error(CODE, 0);                                     \
    }

#define just_path(VALUE) (MaybeJsonPath){JUST, .value=(VALUE)}
#define input_index(INPUT) input_position((INPUT)).index


static inline JsonPath *build_path(SyntaxNode *node)
{
    if(NULL == node)
    {
        return NULL;
    }
    return NULL;
}

static inline MaybeJsonPath transform(MaybeSyntaxNode *node, Input *input)
{
    if(is_nothing(*node))
    {
        return path_error(node->code, input_index(input));
    }
    return just_path(build_path(node->value));

}

MaybeJsonPath read_path(const String *expression)
{
    PRECOND_ELSE_NOTHING(ERR_PARSER_EMPTY_INPUT, NULL != expression);
    PRECOND_ELSE_NOTHING(ERR_PARSER_EMPTY_INPUT, 0 != string_length(expression));

    parser_debug("parsing expression: '%s'", cstr(expression));

    MaybeJsonPath result;

    Input *input = make_input_from_string(expression);
    if(NULL == input)
    {
        result = path_error(ERR_PARSER_OUT_OF_MEMORY, 0);
        goto exit;
    }
    Parser *parser = jsonpath();
    if(NULL == parser)
    {
        result = path_error(ERR_PARSER_OUT_OF_MEMORY, 0);
        goto exit;
    }
    MaybeSyntaxNode root = parse(parser, input);
    /* if(input_has_more(input)) */
    /* { */
    /*     result = path_error(ERR_PARSER_UNEXPECTED_VALUE, input_position(input)); */
    /*     goto cleanup; */
    /* } */
    if(is_nothing(root))
    {
        result = path_error(code(root), input_index(input));
        goto cleanup;
    }

    result = transform(&root, input);

  cleanup:
    // xxx - free parser!
    //parser_free(parser);
    dispose_input(input);
  exit:
    return result;
}
