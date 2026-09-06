#ifndef REVEAL_H
#define REVEAL_H

#include "lexer.h"
#include "hop.h"

int read_directory(char *path , int show_all , int is_recursive , char *base_path);

int reveal(char *home_dir , char *prev_dir , char *curr_dir , Token *head);

#endif