#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<errno.h>
#include<signal.h>
#include<fcntl.h>
#include"../include/lexer.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/peek.h"
#include "../include/locate.h"
#include "../include/activities.h"
#include "../include/redirect.h"

#include"../include/execute.h"

JobTrack tracked_jobs[ARGS_MAX];
int tracked_job_count;
volatile sig_atomic_t foreground_running = 0;

int has_stopped_jobs() {
    for (int i = 0; i < tracked_job_count; i++) {
        for (int j = 0; j < tracked_jobs[i].procs_count; j++) {
            if (tracked_jobs[i].procs[j].status == STOPPED) {
                return 1;
            }
        }
    }
    return 0;
}

void kill_all_jobs() {
    for (int i = 0; i < tracked_job_count; i++) {
        kill(-tracked_jobs[i].pgid, SIGHUP);
        kill(-tracked_jobs[i].pgid, SIGCONT);
    }
}

void safe_print(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

void safe_print_int(int num) {
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    if (num == 0) {
        safe_print("0");
        return;
    }
    while (num > 0 && i >= 0) {
        buf[i] = (num % 10) + '0';
        num /= 10;
        i--;
    }
    safe_print(&buf[i + 1]);
}

void print_job_completion(Process *job){
    safe_print(job->command);
    safe_print(" with pid ");
    safe_print_int(job->pid);
    
    if(WIFEXITED(job->exit_status)){
        safe_print(" exited normally\n");
    }else if(WIFSIGNALED(job->exit_status)){
        safe_print(" exited abnormally\n");
    }
}

void print_job_stopped_or_running(JobTrack *job , int is_stopped){
    printf("[%d] + %s   " , job->job_id , (is_stopped ? "Stopped" : "Running"));

    for(int i = 0 ; i < job->procs_count ; i++){
        printf("%s ", job->procs[i].command);
    }
    printf("\n");
}

void print_completed_jobs(){
    for(int i = 0 ; i < tracked_job_count ; i++){
        if(tracked_jobs[i].is_background){
            int all_completed = 1;
        
            for(int j = 0 ; j < tracked_jobs[i].procs_count ; j++){
                if(tracked_jobs[i].procs[j].status == COMPLETED_BUT_NOT_REPORTED){
                    // Only print completion message for the FIRST process in the pipeline (index 0)
                    if(j == 0) {
                        print_job_completion(&(tracked_jobs[i].procs[j]));
                    }
                    tracked_jobs[i].procs[j].status = COMPLETED;
                }
                // If any process is NOT completed, the job as a whole isn't done yet
                if(tracked_jobs[i].procs[j].status != COMPLETED) {
                    all_completed = 0;
                }
            }
            
            // If every process in the pipeline is fully COMPLETED, remove the job!
            if (all_completed) {
                for(int k = i; k < tracked_job_count - 1; k++){
                    tracked_jobs[k] = tracked_jobs[k+1];
                }
                tracked_job_count--;
                i--; // adjust index after shift
            }
        }
    }
}

void sigchld_handler(int sig){
    int saved_errno = errno;
    int status;
    pid_t pid ;

    while((pid = waitpid(-1 , &status , WNOHANG | WUNTRACED | WCONTINUED)) > 0){
        // now find the this pid in the bg job array
        for(int i = 0 ; i < tracked_job_count ; i++){

            for(int j = 0 ; j < tracked_jobs[i].procs_count ; j++){
                if(tracked_jobs[i].procs[j].pid == pid){
                    if(WIFEXITED(status) || WIFSIGNALED(status)){
                        tracked_jobs[i].procs[j].exit_status = status;
                        tracked_jobs[i].procs[j].status = COMPLETED_BUT_NOT_REPORTED;
                    } else if(WIFSTOPPED(status)){
                        tracked_jobs[i].procs[j].status = STOPPED;
                    } else if(WIFCONTINUED(status)){
                        tracked_jobs[i].procs[j].status = RUNNING;
                    }
                    break;
                }
            }
        }
    }

    print_completed_jobs();
    errno = saved_errno;
}

void execute_command(Token *head , char *full_path){
    // now head's text is full file path, just need to make a child process and execute it
    Token *t;

    char *args[ARGS_MAX];
    args[0] = full_path;
    int i = 1;
    t = head->next;
    while(t != NULL && i < ARGS_MAX - 1){
        if(t->type == OP_LT || t->type == OP_GT || t->type == OP_GTGT){
            if(t->next != NULL){
                t = t->next->next;
            }else{
                t = t->next;
            }
        }else if(t->type == WORD){
            args[i++] = t->text;
            t = t->next;
        }else{
            break;
        }
    }
    args[i] = NULL;

    if(execv(full_path , args) == -1){
        printf("cshell: command not found (%s)\n" , head->text);
        exit(-1);
    }
}

void process_command_path(Token *head){
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
            exit(-1);
        }

        return ;
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
                    closedir(dir);
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
            free(path_copy);
            execute_command(head , full_path);
            return;
        }
        current_dir = strtok(NULL , ":");
    }

    free(path_copy);
    printf("cshell: command not found (%s)\n" , head->text);
    exit(-1);
}

int job_counter = 1;

int execute_pipe(Token *command_arr[] , int num_commands,char *home_dir , char *prev_dir , char *curr_dir, int is_background , int job_number){
    int i = 0;
    int failed = 0;
    int child_process_count = 0;
    pid_t foreground_pids[ARGS_MAX];
    int last_pipe_read = -1;
    foreground_running = 1;

    int job_number_printed = 0;
    sigset_t mask , prev_mask;
    sigemptyset(&mask);
    sigaddset(&mask , SIGCHLD);
    sigprocmask(SIG_BLOCK , &mask , &prev_mask);
    pid_t pgid;
    while(i < num_commands){
        if(command_arr[i] != NULL && command_arr[i]->type == WORD && strcmp(command_arr[i]->text, "hop") == 0) {
            if(hop(home_dir, prev_dir, curr_dir, command_arr[i]) != 0){
                break;
            }
            i++;
            continue;
        }else if(command_arr[i] != NULL && command_arr[i]->type == WORD && strcmp(command_arr[i]->text, "cd") == 0){
            char *target_dir = home_dir; // Default to home if no argument
            if (command_arr[i]->next != NULL) {
                target_dir = command_arr[i]->next->text;
            }
            if (chdir(target_dir) != 0) {
                perror("cshell");
            }
            i++;
            continue;
        }else if(command_arr[i] != NULL && command_arr[i]->type == WORD && strcmp(command_arr[i]->text, "exit") == 0){
            printf("Exiting...\n");
            i++;
            kill_all_jobs();
            exit(0);
        }

        int pipefd[2];
        pipe(pipefd);
        Token *command = command_arr[i];
        pid_t p = fork();

        foreground_pids[child_process_count] = p;
        child_process_count++;
        
        if(p == 0){
            signal(SIGINT , SIG_DFL);
            signal(SIGTSTP , SIG_DFL);
            signal(SIGTTOU , SIG_DFL);

            if(i == 0){
                setpgid(0 ,0);
            }else{
                setpgid(0 , pgid);
            }

            sigprocmask(SIG_SETMASK , &prev_mask , NULL);
            if(i > 0){
                // connect this child's input to the previous pipe's output
                dup2(last_pipe_read , STDIN_FILENO);
            }
            if(i+1 < num_commands){
                // if not the last command connect it's output to next output 
                dup2(pipefd[1] , STDOUT_FILENO);
            }

            // todo : close the unused file descriptors
            close(pipefd[0]);
            close(pipefd[1]);
            if(last_pipe_read != -1){
                close(last_pipe_read);
            }

            Token *rt = command;
            int input_handled = 0 , output_handled = 0;
            while(rt != NULL){   
                if(rt->type == OP_LT && !input_handled){
                    handle_input_redirection(command);
                    input_handled = 1;
                }
                if((rt->type == OP_GT || rt->type == OP_GTGT) && !output_handled){
                    handle_output_redirection(command);
                    output_handled = 1;
                }
                rt = rt->next;
            }

            if(command != NULL && command->type == WORD && strcmp(command->text , "reveal") == 0){
                if(reveal(home_dir , prev_dir , curr_dir , command) != 0){
                    failed = 1;
                }
            }else if(command != NULL && command->type == WORD && strcmp(command->text , "peek") == 0){
                if(peek(command , home_dir) != 0){
                    failed = 1;
                }
                
            }else if(command != NULL && command->type == WORD && strcmp(command->text , "locate") == 0){
                if(locate(command) != 0){
                    failed = 1;
                }
            }else if(command != NULL && command->type == WORD && strcmp(command->text , "activities") == 0){
                show_activities();
            }else{
                // this is a different command , i have to check the current directory for this exec or the path for this directory
                
                process_command_path(command);
            }
            exit(failed ? -1 : 0);
        }else{
            if(i > 0) setpgid(p , pgid);
            else pgid = p;

            if(!job_number_printed && is_background){
                printf("[%d] %d\n" , job_number , p);
                job_number_printed  = 1;
            }
            tracked_jobs[tracked_job_count].is_background = is_background;
            tracked_jobs[tracked_job_count].pgid = pgid;
            tracked_jobs[tracked_job_count].job_id = job_number;
            tracked_jobs[tracked_job_count].procs[i].pid=p;
            strcpy(tracked_jobs[tracked_job_count].procs[i].command , command_arr[i]->text);
            tracked_jobs[tracked_job_count].procs[i].status=RUNNING;
            fflush(stdout);
            
            if (last_pipe_read != -1) {
                close(last_pipe_read);
            }
            
            if(i+1 < num_commands){
                close(pipefd[1]);
                
                last_pipe_read = pipefd[0];
            }else{
                close(pipefd[1]);
                close(pipefd[0]);
            }
        }
        i++;
    }
    if(child_process_count > 0){
        tracked_jobs[tracked_job_count].procs_count = i;
        tracked_job_count++;
        if(!is_background){
            tcsetpgrp(STDIN_FILENO , pgid);
        }
    }
    for(int j = 1 ; j < num_commands ; j++){
        Token *t = command_arr[j];
        while(t != NULL){
            Token *temp = t;
            t = t->next;
            free(temp);
        }
    }
    int status;
    failed = 0;
    if(!is_background && child_process_count > 0){
        int stopped = 0;
        for(int j = 0 ; j < child_process_count ; j++){
            waitpid(foreground_pids[j] , &status , WUNTRACED);
            if(WIFEXITED(status)){
                int exit_code = WEXITSTATUS(status);
                if(exit_code != 0){
                    failed = 1;
                }
            }else if(WIFSTOPPED(status)){
                stopped = 1;
                tracked_jobs[tracked_job_count-1].job_id = job_counter++;
                tracked_jobs[tracked_job_count-1].is_background = 1;
                for(int k = 0; k < tracked_jobs[tracked_job_count-1].procs_count; k++){
                    tracked_jobs[tracked_job_count-1].procs[k].status = STOPPED;
                }
                print_job_stopped_or_running(&(tracked_jobs[tracked_job_count-1]) , 1);
                break;
            }else if(WIFSIGNALED(status)){
                int sig = WTERMSIG(status);
                if(sig == SIGINT){
                    failed = 1;
                    break;
                }
            }
        }
        if(!stopped){
            tracked_job_count--;
        }
    }
    foreground_running = 0;
    if(!is_background && child_process_count > 0){
        tcsetpgrp(STDIN_FILENO , getpgid(0));
    }
    fflush(stdout);
    sigprocmask(SIG_SETMASK , &prev_mask , NULL);
    return failed ? -1 : 0;
}




void execute(Token *command_list , char *home_dir , char *prev_dir , char *curr_dir){
    job* job_arr[ARGS_MAX];
    for(int i = 0 ; i < ARGS_MAX ; i++){
        job_arr[i] = malloc(sizeof(job));
    }
    Token *t = command_list , *prev = NULL;
    int i = 0,j = 0; // i for job array and j for individual command array
    Token *st = t;

    while(t != NULL){
        if(t->type == OP_PIPE){
            job_arr[i]->command_list[j] = st;
            j++;
            prev->next = NULL;
            st = t->next;
        }else if(t->type == OP_SEMI){
            job_arr[i]->command_list[j] = st;
            job_arr[i]->job_type = JOB_FOREGROUND;
            job_arr[i]->num_commands = j+1;
            j = 0;
            i++;
            prev->next = NULL;
            st = t->next;
        }else if(t->type == OP_AMP){
            job_arr[i]->command_list[j] = st;
            job_arr[i]->job_type = JOB_BACKGROUND;
            job_arr[i]->num_commands = j+1;
            j  = 0 ;
            i++;
            prev->next = NULL;
            st = t->next;
        }
        prev = t;
        t = t->next;
    }
    
    
    if(st != NULL && i < ARGS_MAX){
        job_arr[i]->command_list[j] = st;
        job_arr[i]->job_type = JOB_FOREGROUND;
        job_arr[i]->num_commands = j+1;
        j = 0;
        i++;
        prev->next = NULL;
    }
    
    int num_jobs = i;
    i = 0 ;

    while(i < num_jobs){
        if(job_arr[i]->job_type == JOB_FOREGROUND){
            if(execute_pipe(job_arr[i]->command_list , job_arr[i]->num_commands , home_dir , prev_dir , curr_dir , 0 , 0) != 0){
                break;
            }
        }else{
            job_arr[i]->job_number = job_counter;
            execute_pipe(job_arr[i]->command_list , job_arr[i]->num_commands , home_dir , prev_dir,  curr_dir , 1 , job_counter);
            job_counter++;
        }
        i++;
    }
    for(int k = 0 ; k < ARGS_MAX ; k++){
        free(job_arr[k]);
    }
}