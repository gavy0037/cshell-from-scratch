#ifndef REVEAL_H
#define REVEAL_H

#include "lexer.h"
#include "hop.h"

void read_directory(char *path , int show_all , int is_recursive , char *base_path);

void reveal(char *home_dir , char *prev_dir , char *curr_dir , Token *head);

#endif