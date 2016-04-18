
#include "parser/location.h"
#include "parser/base.h"


struct term_parser_s
{
    Parser           base;
    String          *stop;
};

typedef struct term_parser_s TermParser;


MaybeString default_filter(Input *input)
{
    uint8_t current = input_consume_one(input);
    MutableString *result = make_mstring_with_char(current);
    if(NULL == result)
    {
        return nothing_string(ERR_PARSER_OUT_OF_MEMORY);
    }
    return just_string(result);
}

static void string_parser_free(Parser *parser)
{
    TermParser *self = (TermParser *)parser;
    string_free(self->stop);
    free(self);
}

static inline bool is_stop_char(TermParser *self, uint8_t value)
{
    return string_contains(self->stop, value);
}

static MaybeSyntaxNode string_delegate(Parser *parser, MaybeSyntaxNode node, Input *input)
{
    input_skip_whitespace(input);
    ensure_more_input(input);

    TermParser *self = (TermParser *)parser;

    MutableString *result = make_mstring(4);
    while(true)
    {
        if(!input_has_more(input) || is_stop_char(self, input_peek(input)))
        {
            break;
        }
        MaybeString maybe = self->filter(input);
        if(is_nothing(maybe))
        {
            mstring_free(result);
            return nothing_node(code(maybe));
        }
        mstring_append(&result, value(maybe));
    }

    String *term = mstring_as_string(result);
    SyntaxNode *term_node = make_syntax_node(CST_TERM, term, location_from_input(input));
    syntax_node_add_child(value(node), term_node);
    mstring_free(result);

    return node;
}

Parser *term(const char *stop_characters)
{
    TermParser *self = calloc(1, sizeof(TermParser));
    if(NULL == self)
    {
        return NULL;
    }

    parser_init((Parser *)self, TERM);
    self->base.vtable.delegate = string_delegate;
    self->base.vtable.free = string_parser_free;
    self->stop = make_string(stop_characters);

    return (Parser *)self;
}
