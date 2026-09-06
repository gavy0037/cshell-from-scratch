#ifndef PEEK_H
#define PEEK_H

#include<stdio.h>
#include<stdlib.h>
#include"../include/lexer.h"

#define CHUNK_SIZE 4096

void read_stdin(int is_reverse ,int is_line);
int read_file(int is_reverse , int is_line , char *file_path , char *home_dir);
int peek(Token *head , char *home_dir);
#endif