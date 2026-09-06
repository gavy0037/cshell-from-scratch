#ifndef HOP_H
#define HOP_H

#include"../include/lexer.h"

typedef struct FrecencyNode {
    char *path;
    int frequency;
    struct FrecencyNode *next;
} FrecencyNode;

typedef struct {
    FrecencyNode *node;
    int score;
} FrecencyMatch;

char *resolve_path(char *path , char *prev_dir , char *home_dir , char *curr_dir);
int hop(char *home_dir , char *prev_dir , char *curr_dir , Token *head);
void update_frecency(char *shell_home, char *abs_path);
int fallback_hop(char *shell_home, char *target);

#endif