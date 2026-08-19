#ifndef COMMAND_H
#define COMMAND_H

#include"lexer.h"

void execute_command(Token *head , char *full_path);
void process_command(Token *head);

#endif