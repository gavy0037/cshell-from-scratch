#ifndef SPY_H
#define SPY_H


#include<unistd.h>
#include<lexer.h>
#include<execute.h>

int validate_spy_syntax(Token *cmd);
char *classify(mode_t mode);
void spy_process(Token *cmd);

#endif