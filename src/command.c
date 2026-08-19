#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include"../include/lexer.h"
#define ARGS_MAX 256

void execute_command(Token *head , char *full_path){
    // now head's text is full file path, just need to make a child process and execute it

    char *args[ARGS_MAX];
    Token *t = head->next;
    int i = 1 ;
    args[0] = full_path;
    while(t != NULL && i < ARGS_MAX){
        args[i++] = t->text;
        t = t->next;
    }

    if(i < ARGS_MAX) args[i] = NULL;
    else args[ARGS_MAX-1] = NULL;

    __pid_t x = fork();

    if(x == 0){
        // this is the child
        if(execv(full_path , args) != -1){
            exit(0);
        }else{
            printf("cshell: command not found (%s)\n" , head->text);
            exit(-1);
        }
    }else{
        wait(NULL);
    }
}

void process_command(Token *head){
    // first i need to think if this head command has already a path
    int is_path = 0;
    for(int i = 0 ; i < (int)strlen(head->text) ; i++){
        if(head->text[i] == '/'){
            is_path = 1;
            break;
        }
    }

    if(is_path){
        struct stat st;
        if(stat(head->text , &st) == 0 && S_ISREG(st.st_mode) && access(head->text , X_OK) == 0){
            execute_command(head , head->text);
        }else{
            printf("cshell: command not found (%s)\n" , head->text);
        }

        return;
    }
    // now search locally
    if(head->text[0] != '%'){
        char curr_dir[4096];
        getcwd(curr_dir , sizeof(curr_dir));
        DIR *dir = opendir(curr_dir);
        struct dirent *entry;
        while((entry = readdir(dir)) != NULL){
            if(strcmp(head->text , entry->d_name) == 0){
                struct stat st;
                char full_file_path[8192];
                snprintf(full_file_path , 8192 , "%s/%s" , curr_dir , head->text);
                if(stat(full_file_path , &st) == 0 && S_ISREG(st.st_mode) && access(full_file_path , X_OK) == 0){
                    execute_command(head,  full_file_path);
    
                    return;
                }
            }
        }
        closedir(dir);
    }

    if(head->text[0] == '%'){
        memmove(head->text , head->text+1 , strlen(head->text));
    }
    // search in path
    char *path = getenv("PATH");
    char *path_copy = strdup(path);

    char *current_dir = strtok(path_copy , ":");
    struct stat path_stat;
    while(current_dir != NULL){
        char full_path[8192];
        snprintf(full_path , 8192 , "%s/%s" , current_dir , head->text);
        if(stat(full_path , &path_stat) == 0 && S_ISREG(path_stat.st_mode) && access(full_path , X_OK) == 0){
            execute_command(head , full_path);
            return;
        }
        current_dir = strtok(NULL , ":");
    }

    printf("cshell: command not found (%s)\n" , head->text);
}