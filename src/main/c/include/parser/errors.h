#pragma once

enum parser_error_e
{
    PREMATURE_END_OF_INPUT,
    UNSUPPORTED_CONTROL_CHARACTER,
    UNSUPPORTED_ESCAPE_SEQUENCE,
};

typedef enum parser_error_e ParserErrorCode;

struct parser_error_s
{
    ParserErrorCode code;
    Position position;
};

typedef struct parser_error_s ParserError;

const char *parser_strerror(ParserErrorCode code);
