#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include"../include/lexer.h"
#include"../include/hop.h"

char *resolve_path(char *path , char *prev_dir , char *home_dir , char *curr_dir){
    if(strlen(path) == 0){
        return curr_dir;
    }else if(path[0] == '~'){
        char temp[4096];
        strcpy(temp , path);
        strcpy(path , home_dir);
        strcat(path , temp);
        return path;
    }else if(path[0] == '-' && path[1] == '\0'){
        return prev_dir;
    }else{
        return path;
    }
}

void hop(char *home_dir , char *prev_dir, char *curr_dir, Token *head){
    Token *t = head->next;
    if(t == NULL){
        getcwd(prev_dir , 4096);
        chdir(home_dir);
        return;
    }
    
    if(t->text[0] == '-' && strlen(prev_dir) == 0){
        getcwd(prev_dir , 4096);
        return;
    }
    char buffer[4096];
    while(t != NULL){
        getcwd(buffer , 4096);
        strcpy(curr_dir, resolve_path(t->text , prev_dir , home_dir , curr_dir));
        if(chdir(curr_dir) == 0){
            strcpy(prev_dir , buffer);
        }
        t = t->next;
    }
}