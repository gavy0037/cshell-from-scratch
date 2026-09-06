#ifndef COMMAND_H
#define COMMAND_H


#include"lexer.h"


#define MAX_CMDS 64
#define ARGS_MAX 256


typedef enum JOB_T{
    JOB_FOREGROUND,
    JOB_BACKGROUND,
} JOB_T;

typedef struct job{
    pid_t pid;
    int job_number ;
    JOB_T job_type;
    Token *command_list[MAX_CMDS];
    int num_commands;

}job;

typedef struct{
    pid_t pid ;
    int job_id ;
    char command[MAX_CMDS];
    int is_active;
    int completed ;
    int exit_status;
} Background_job;


int execute_pipe(Token *command_arr[] , int num_commands ,char *home_dir , char *prev_dir , char *curr_dir);

void execute_command(Token *head , char *full_path);
void process_command_path(Token *head);
void execute(Token *command_list , char *home_dir , char *prev_dir , char *curr_dir);
void print_job_completion(Background_job *bg_job);
void sigchld_handler(int sig);

#endif