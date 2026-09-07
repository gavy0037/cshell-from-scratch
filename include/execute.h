#ifndef COMMAND_H
#define COMMAND_H


#include"lexer.h"

#define MAX_CMD_SIZE 1024
#define MAX_CMDS 64
#define ARGS_MAX 256

typedef enum JOB_T{
    JOB_FOREGROUND,
    JOB_BACKGROUND,
} JOB_T;

typedef enum{
    RUNNING,
    STOPPED,
    COMPLETED_BUT_NOT_REPORTED,
    COMPLETED
} JOB_STATUS;

typedef struct job{
    pid_t pid;
    int job_number ;
    JOB_T job_type;
    Token *command_list[MAX_CMDS];
    int num_commands;
}job;

typedef struct{
    pid_t pid;
    char command[MAX_CMD_SIZE];
    JOB_STATUS status;
    int exit_status;
} Process;

typedef struct {
    int job_id;
    pid_t pgid;
    Process procs[MAX_CMDS];
    int procs_count;
    int is_background;
    char full_job_command[MAX_CMD_SIZE];
} JobTrack;

extern JobTrack tracked_jobs[ARGS_MAX];
extern int tracked_job_count;
extern int job_counter;

int has_stopped_jobs();
void kill_all_jobs();
void print_job_stopped_or_running(JobTrack *job , int is_stopped);

void print_completed_jobs();
int execute_pipe(Token *command_arr[] , int num_commands ,char *home_dir , char *prev_dir , char *curr_dir , int is_background, int job_number);

void execute_command(Token *head , char *full_path);
void process_command_path(Token *head);
void execute(Token *command_list , char *home_dir , char *prev_dir , char *curr_dir);
void print_job_completion(Process *bg_job);
void sigchld_handler(int sig);

#endif