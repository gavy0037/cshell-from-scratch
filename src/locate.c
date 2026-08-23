#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/stat.h>
#include"../include/locate.h"
#include"../include/lexer.h"

int search_path_for_exe(char *file_name){
    char *path = getenv("PATH");
    if (!path) return 0;
    int success = 0;
    char *path_copy = strdup(path);

    char *current_dir = strtok(path_copy , ":");
    struct stat path_stat;
    while(current_dir != NULL){
        char full_path[8192];
        snprintf(full_path , 8192 , "%s/%s" , current_dir , file_name);
        if(stat(full_path , &path_stat) == 0 && S_ISREG(path_stat.st_mode) && access(full_path , X_OK) == 0){
            success = 1;
            printf("%s\n" , full_path);
        }
        current_dir = strtok(NULL , ":");
    }

    free(path_copy);
    return success;
}

void locate(Token *head){
    char curr_dir[4096];
    getcwd(curr_dir ,sizeof(curr_dir));
    struct stat path_stat;
    Token *t = head->next;
    int arg_count = 0;
    while(t != NULL){
        if(t->type == OP_LT || t->type == OP_GT || t->type == OP_GTGT){
            if(t->next != NULL) t = t->next->next;
            else t = t->next;
            continue;
        }
        if(t->type == WORD){
            arg_count++;
            int success = 0;
            char file_path[8192];
            snprintf(file_path , 8192 , "%s/%s" , curr_dir , t->text);

            if(stat(file_path , &path_stat) == 0 && S_ISREG(path_stat.st_mode) && access(file_path , X_OK) == 0){
                success = 1;
                printf("%s\n" , file_path);
            }
            success += search_path_for_exe(t->text);
            
            if(success == 0){
                printf("locate: command not found (%s)\n" , t->text);
            }
            t = t->next;
        }else{
            break;
        }
    }
    if(arg_count == 0){
        printf("locate: invalid syntax\n");
    }
}