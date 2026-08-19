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

    return success;
}

void locate(Token *head){
    char curr_dir[4096];
    getcwd(curr_dir ,sizeof(curr_dir));
    struct stat path_stat ;
    Token *t = head->next;
    while(t != NULL){
        int success = 0;
        char file_path[8192];
        snprintf(file_path , 8192 , "%s/%s" , curr_dir , t->text);

        if(stat(file_path , &path_stat) == 0 && S_ISREG(path_stat.st_mode) && access(file_path , X_OK) == 0){
            success = 1;
            printf("%s\n" , file_path);
        }
        success+=search_path_for_exe(t->text);
        
        if(success == 0){
            printf("locate: command not found (%s)\n" , t->text);
        }
        t = t->next;
    }
}