#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"

#define ERR -1
#define ACC 99 // Accept: Valid command reached Epsilon (end of line)

// The abstract states of the parser
typedef enum {
    LINE = 0,
    ARG  = 1,
    CMD  = 2,
    TGT  = 3,
    BG   = 4
} ParserState;

int validate_syntax(Token *token_list);

#endif