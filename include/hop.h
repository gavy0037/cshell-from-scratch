#ifndef HOP_H
#define HOP_H

#include"../include/lexer.h"

char *resolve_path(char *path , char *prev_dir , char *home_dir);
void hop(char *home_dir , char *prev_dir , char *curr_dir , Token *head);

#endif