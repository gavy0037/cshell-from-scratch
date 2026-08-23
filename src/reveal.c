#define _DEFAULT_SOURCE  // Activates modern BSD/SVID extensions
#define _GNU_SOURCE 
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<string.h>
#include"../include/reveal.h"
#include"../include/hop.h"
#include "../include/lexer.h"

int ascii_sort(const struct dirent **a, const struct dirent **b) { // the standard alpha sort for scandir was not working
    return strcmp((*a)->d_name, (*b)->d_name);
}

void read_directory(char *path , int show_all , int is_recursive , char *base_path){
    struct dirent **namelist;
    
    int n = scandir(path, &namelist, NULL, ascii_sort);
    
    if(n < 0){
        printf("reveal: no such directory\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        struct dirent *entry = namelist[i];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){                                                                                                           
            free(namelist[i]);
            continue;
        }
        if (!(entry->d_name[0] == '.' && !show_all)) {
            
            char full_entry_path[8192];
            snprintf(full_entry_path, sizeof(full_entry_path), "%s/%s", path, entry->d_name);
            
            struct stat st;
            int is_dir = (stat(full_entry_path, &st) == 0 && S_ISDIR(st.st_mode));
            
            int has_space = (strchr(entry->d_name, ' ') != NULL);
            
            if (is_dir && is_recursive) {
                if (has_space) {
                    printf("%s'%s'/\n", base_path, entry->d_name);
                } else {
                    printf("%s%s/\n", base_path, entry->d_name);
                }
            } else {
                if (has_space) {
                    printf("%s'%s'\n", base_path, entry->d_name);
                } else {
                    printf("%s%s\n", base_path, entry->d_name);
                }
            }
            
            if(is_dir && is_recursive){
                if(strcmp(entry->d_name , ".") != 0 && strcmp(entry->d_name , "..") != 0){
                    char next_base[8192];
                    if(strlen(base_path) == 0){
                        snprintf(next_base, sizeof(next_base), "%s/", entry->d_name);
                    }else{
                        snprintf(next_base, sizeof(next_base), "%s%s/", base_path, entry->d_name);
                    }

                    read_directory(full_entry_path , show_all , is_recursive , next_base);
                }
            }
        }
        
        free(namelist[i]); 
    }
    
    free(namelist); 
}


void reveal(char *home_dir , char *prev_dir , char *curr_dir , Token *head){
    Token *t = head->next;
    int is_recursive = 0 , show_all = 0;
    char *target_path = NULL;

    while(t != NULL){
        if(t->type == OP_LT || t->type == OP_GT || t->type == OP_GTGT){
            if(t->next != NULL) t = t->next->next;
            else t = t->next;
            continue;
        }
        if(t->type == WORD){
            if(t->text[0] == '-' && strlen(t->text) > 1 && target_path == NULL){
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
            }else{
                if(target_path != NULL){
                    printf("reveal: invalid syntax\n");
                    return;
                }
                target_path = t->text;
            }
            t = t->next;
        }else{
            break;
        }
    }

    char path_to_inspect[4096];
    if(target_path == NULL){
        strcpy(path_to_inspect, resolve_path("" , prev_dir ,home_dir ,curr_dir));
        read_directory(path_to_inspect , show_all , is_recursive , "");
    }else{
        if(target_path[0] == '-' && strlen(prev_dir) == 0){
            printf("reveal: no such directory\n");
            return;
        }
        strcpy(path_to_inspect , resolve_path(target_path ,prev_dir , home_dir ,curr_dir));
        read_directory(path_to_inspect , show_all , is_recursive , "");
    }
}