
#pragma once


#include "maybe.h"
#include "str.h"

#include "parser/location.h"
#include "parser/syntax.h"


/* Parser Results  */

enum parser_result_code_e
{
    PARSER_SUCCESS = 0,
    ERR_PARSER_OUT_OF_MEMORY,     // unable to allocate memory
    ERR_PARSER_EMPTY_INPUT,       // no input to parse
    ERR_PARSER_END_OF_INPUT,      // premature end of input
    ERR_PARSER_UNEXPECTED_VALUE,  // expected one value but found another
};

typedef enum parser_result_code_e ParserResultCode;

typedef char *(*StatusMessageFormatter)(uint_fast16_t code, SourceLocation location);


/* Parser Entities */

typedef struct parser_s Parser;


/* Parser API */

Maybe parse(Parser *parser, Input *input);


/* Parser Destructor */

void parser_free(Parser *value);


/* Non-Terminal Grammar Parsers */

typedef Maybe (*tree_rewriter)(Maybe node);

Parser *rule_parser(const char *name, Parser *expression, tree_rewriter rewriter);
#define rule(EXPR, FUNC) rule_parser(__func__, (EXPR), (FUNC))
#define simple_rule(EXPR) rule((EXPR), NULL)

Parser *choice_parser(Parser *one, Parser *two, ...);
#define choice(ONE, TWO, ...) choice_parser((ONE), (TWO), ##__VA_ARGS__, NULL)

Parser *sequence_parser(Parser *one, Parser *two, ...);
#define sequence(ONE, TWO, ...) sequence_parser((ONE), (TWO), ##__VA_ARGS__, NULL)

Parser *option(Parser *optional);
Parser *repetition(Parser *repeated);

Parser *reference(const char *value);


/* Terminal Grammar Parsers */

Parser *literal(const char *value);

Parser *number(void);
Parser *integer(void);
Parser *signed_integer(void);
Parser *non_zero_signed_integer(void);

Parser *quoted_string(char quote);
Parser *term(const char *stop_characters);
