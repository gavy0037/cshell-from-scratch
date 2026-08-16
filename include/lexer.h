#ifndef LEXER_H
#define LEXER_H

typedef enum {
    OP_PIPE,   // |
    OP_AMP,    // &
    OP_SEMI,   // ;
    OP_LT,     // <
    OP_GT,     // >
    OP_GTGT,   // >>
    WORD       // commands, args, files
} TokenType;

typedef struct Token {
    TokenType type;
    char *text; // Only relevant if the type is WORD
    struct Token *next;
} Token;

// State machine definitions
typedef enum {
    IN_WORD,
    NORMAL,
    IN_SQ,  // Single quotes ''
    IN_DQ   // Double quotes ""
} State;

Token *tokenize(char *input);

#endif