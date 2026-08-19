#ifndef COMMAND_H
#define COMMAND_H

#include"lexer.h"

void execute_command(Token *head , char *full_path);
void process_command_path(Token *head);
void execute(Token *command_list , char *home_dir , char *prev_dir , char *curr_dir);

#endif