#include"../include/parser.h"
#include"../include/lexer.h"
#include<stdio.h>

int transition_table[5][8] = {
    // PIPE | AMP | SEMI| LT  | GT  | GTGT| WORD| EP
    {  ERR,   ERR,  ERR,  ERR,  ERR,  ERR,  1,    ACC }, // 0: STATE_LINE
    {  2,     4,    2,    3,    3,    3,    1,    ACC }, // 1: STATE_ARG
    {  ERR,   ERR,  ERR,  ERR,  ERR,  ERR,  1,    ERR }, // 2: STATE_CMD
    {  ERR,   ERR,  ERR,  ERR,  ERR,  ERR,  1,    ERR }, // 3: STATE_TGT
    {  ERR,   ERR,  ERR,  ERR,  ERR,  ERR,  1,    ACC }  // 4: STATE_BG
};

int validate_syntax(Token *token_list){

    if(token_list == NULL){
        return -1;
    }
    Token *token_pointer = token_list;

    ParserState p_state = LINE;

    while(token_pointer != NULL){
        TokenType t = token_pointer->type;
        int next_state = transition_table[p_state][t];

        if(next_state == ERR){
            printf("cshell: invalid syntax\n");
            return -1;
        }

        p_state = (ParserState)next_state;
        token_pointer = token_pointer->next;
    }

    if(transition_table[p_state][7] == ERR){
        printf("cshell: invalid syntax\n");
        return -1;
    }

    return 1;
}