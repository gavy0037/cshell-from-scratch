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
#include "../include/redirect.h"

#include"../include/execute.h"

Background_job bg_jobs[ARGS_MAX];
int bg_job_count = 0;
volatile sig_atomic_t foreground_running = 0;

void print_job_completion(Background_job *bg_job){
    if(WIFEXITED(bg_job->exit_status)){
        printf("%s with pid %d exited normally\n" , bg_job->command , bg_job->pid);
    }else if(WIFSIGNALED(bg_job->exit_status)){
        printf("%s with pid %d exited abnormally\n" , bg_job->command , bg_job->pid);
    }
    fflush(stdout);
}

void sigchld_handler(int sig){
    int saved_errno = errno;
    int status;
    pid_t pid ;

    while((pid = waitpid(-1 , &status , WNOHANG)) > 0){
        // now find the this pid in the bg job array
        for(int i = 0 ; i < bg_job_count ; i++){
            if(bg_jobs[i].pid == pid && bg_jobs[i].is_active){
                bg_jobs[i].is_active = 0;
                bg_jobs[i].exit_status = status;

                if(foreground_running){
                    bg_jobs[i].completed = 1;
                }else{
                    print_job_completion(&bg_jobs[i]);
                }
                break;
            }
        }
    }

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

int execute_pipe(Token *command_arr[] , int num_commands,char *home_dir , char *prev_dir , char *curr_dir, int is_background , int job_number){
    int i = 0 ;
    int failed = 0;
    int child_process_count = 0;
    pid_t foreground_pids[ARGS_MAX];
    int last_pipe_read = -1;
    foreground_running = 1;

    int job_number_printed = 0;
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
            exit(0);
        }

        int pipefd[2];
        pipe(pipefd);
        Token *command = command_arr[i];
        pid_t p = fork();
        foreground_pids[child_process_count] = p;
        child_process_count++;

        sigset_t mask , prev_mask;
        if(is_background){
            sigemptyset(&mask);
            sigaddset(&mask , SIGCHLD);
            sigprocmask(SIG_BLOCK , &mask , &prev_mask);
        }
        if(p == 0){
            if(is_background) sigprocmask(SIG_SETMASK , &prev_mask , NULL);
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
            }else{
                // this is a different command , i have to check the current directory for this exec or the path for this directory
                
                process_command_path(command);
            }
            exit(failed ? -1 : 0);
        }else{
            if(is_background){
                if(!job_number_printed){
                    printf("[%d] %d\n" , job_number , p);
                    job_number_printed  = 1;
                    bg_jobs[bg_job_count].pid = p;
                    bg_jobs[bg_job_count].job_id = job_number;
                    strcpy(bg_jobs[bg_job_count].command , command_arr[i]->text);
                    bg_jobs[bg_job_count].is_active = 1;
                    bg_jobs[bg_job_count].completed = 0;
                    
                    bg_job_count++;
                }
                
                
                fflush(stdout);
                sigprocmask(SIG_SETMASK , &prev_mask , NULL);
            }

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
    for(int j = 1 ; j < num_commands ; j++){
        Token *t = command_arr[j];
        while(t != NULL){
            Token *temp = t;
            t = t->next;
            free(temp);
        }
    }
    int status;
    failed =0;
    if(!is_background){
        for(int j = 0 ; j < child_process_count ; j++){
            waitpid(foreground_pids[j] , &status , 0);// wait for each command in pipeline so that i don't run the parent when the pipeing is not yet finisehd and also i have spawned every child so my pipeline is also fine.
            if(WIFEXITED(status)){
                int exit_code = WEXITSTATUS(status);
                if(exit_code != 0){
                    failed = 1;
                }
            }
        }
    }
    foreground_running = 0;

    for(int i = 0 ; i < bg_job_count ; i++){
        if(bg_jobs[i].completed){
            print_job_completion(&bg_jobs[i]);
            bg_jobs[i].completed = 0;
        }
    }
    fflush(stdout);
    return failed ? -1 : 0;
}


int job_counter = 1 ;

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