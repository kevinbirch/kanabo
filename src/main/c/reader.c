
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>

#include "reader.h"

int build_tree(yaml_parser_t *parser);
void parser_error(yaml_parser_t *parser);

int read_yaml_file(FILE *istream)
{
    int result = 0;
    yaml_parser_t parser;
    
    memset(&parser, 0, sizeof(parser));

    if (!yaml_parser_initialize(&parser))
    {
        parser_error(&parser);
        yaml_parser_delete(&parser);
        return parser.error;
    }

    yaml_parser_set_input_file(&parser, istream);
    result = build_tree(&parser);
    
    yaml_parser_delete(&parser);
    
    return result;
}

int build_tree(yaml_parser_t *parser)
{
    int result = 0;
    yaml_event_t event;
    
    memset(&event, 0, sizeof(event));

    bool done = false;
    while(!done)
    {
        if (!yaml_parser_parse(parser, &event))
        {
            parser_error(parser);
            result = parser->error;
            break;
        }
            
        if (event.type == YAML_STREAM_END_EVENT)
        {
            done = true;
        }
    }

    return result;
}

void parser_error(yaml_parser_t *parser)
{
    switch (parser->error)
    {
        case YAML_MEMORY_ERROR:
            fprintf(stderr, "Memory error: Not enough memory for parsing\n");
            break;
	
        case YAML_READER_ERROR:
            if (parser->problem_value != -1)
            {
                fprintf(stderr, "Reader error: %s: #%X at %zd\n", parser->problem, parser->problem_value, parser->problem_offset);
            }
            else
            {
                fprintf(stderr, "Reader error: %s at %zd\n", parser->problem, parser->problem_offset);
            }
            break;
	
        case YAML_SCANNER_ERROR:
            if (parser->context)
            {
                fprintf(stderr, "Scanner error: %s at line %ld, column %ld\n"
                        "%s at line %ld, column %ld\n", parser->context,
                        parser->context_mark.line+1, parser->context_mark.column+1,
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            else
            {
                fprintf(stderr, "Scanner error: %s at line %ld, column %ld\n",
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            break;
	
        case YAML_PARSER_ERROR:
            if (parser->context)
            {
                fprintf(stderr, "Parser error: %s at line %ld, column %ld\n"
                        "%s at line %ld, column %ld\n", parser->context,
                        parser->context_mark.line+1, parser->context_mark.column+1,
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            else
            {
                fprintf(stderr, "Parser error: %s at line %ld, column %ld\n",
                        parser->problem, parser->problem_mark.line+1,
                        parser->problem_mark.column+1);
            }
            break;
	
        default:
            /* Couldn't happen. */
            fprintf(stderr, "Internal error\n");
            break;
    }
}
