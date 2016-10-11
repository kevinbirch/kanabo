
#include "log.h"

#include "parser/base.h"
#include "parser/location.h"


Maybe parse(Parser *parser, Input *input)
{
    return parser_execute(parser, input);
}
