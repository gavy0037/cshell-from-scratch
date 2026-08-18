#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<string.h>
#include"../include/reveal.h"
#include"../include/hop.h"
#include "../include/lexer.h"

void read_directory(char *path , int show_all , int is_recursive , char *base_path){
    DIR *dir = opendir(path);
    struct dirent *entry;
    
    while((entry = readdir(dir)) != NULL){
        if(entry->d_name[0] == '.' && !show_all){
            continue;
        }
        
        printf("%s%s\n", base_path , entry->d_name);
        
        char full_entry_path[4096];
        snprintf(full_entry_path, 4096, "%s/%s", path, entry->d_name);
        
        struct stat st;
        if(stat(full_entry_path, &st) == 0 && S_ISDIR(st.st_mode) && is_recursive){
            if(strcmp(entry->d_name , ".") == 0 || strcmp(entry->d_name , "..") == 0){
                continue;
            }
            char next_base[4096];
            if(strlen(base_path) == 0){
                snprintf(next_base, sizeof(next_base), "%s/", entry->d_name);
            }else{
                snprintf(next_base, sizeof(next_base), "%s%s/", base_path, entry->d_name);
            }

            read_directory(full_entry_path , show_all , is_recursive , next_base);
        }
    }
    closedir(dir);
}


void reveal(char *home_dir , char *prev_dir , char *curr_dir , Token *head){
    Token *t = head->next;
    int is_recursive = 0 , show_all = 0;
    if(t != NULL && t->type == WORD){
        for(int i = 1 ; i < (int)strlen(t->text) ; i++){
            if(t->text[i] == 't'){
                is_recursive = 1;
            }else if(t->text[i] == 'a'){
                show_all = 1;
            }else{
                printf("reveal: invalid syntax\n");
                return;
            }
        }
    }
    if(t) t = t->next;
    char path_to_inspect[4096];
    if(t == NULL){
        strcpy(path_to_inspect, resolve_path("" , prev_dir , home_dir));
        read_directory(path_to_inspect , show_all , is_recursive , "");
    }else{
        if(t->text[0] == '-' && strlen(prev_dir) == 0){
            printf("reveal: no such directory\n");
            return;
        }
        strcpy(path_to_inspect , resolve_path(t->text ,prev_dir , home_dir));
        t = t->next;
        if(t != NULL){
            printf("reveal: invalid syntax\n");
            return;
        }
        read_directory(path_to_inspect , show_all , is_recursive , "");
    }
}