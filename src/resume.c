#include<stdio.h>
#include<stdlib.h>

#include"../include/resume.h"
#include"../include/execute.h"
#include"../include/lexer.h"
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

void resume_background(JobTrack *curr_job){
    
    curr_job->is_background = 1;
    
    for(int i = 0 ; i < curr_job->procs_count ; i++){
        if(curr_job->procs[i].status == STOPPED){
            curr_job->procs[i].status = RUNNING;
        }
    }
    
    printf("[%d] + Running   %s\n" , curr_job->job_id, curr_job->full_job_command);
    kill(-curr_job->pgid , SIGCONT);
}

void resume_foreground(JobTrack *curr_job , int is_timer , int timeout){
    
    if(is_timer){
        alarm(timeout);
    }

    curr_job->is_background = 0;
    
    for(int i = 0 ; i < curr_job->procs_count ; i++){
        if(curr_job->procs[i].status == STOPPED){
            curr_job->procs[i].status = RUNNING;
        }
    }
    
    printf("%s\n", curr_job->full_job_command);

    tcsetpgrp(STDIN_FILENO , curr_job->pgid);
    kill(-curr_job->pgid , SIGCONT);
    int status;
    int stopped = 0;
    for(int j = 0 ; j < curr_job->procs_count ; j++){
        if(curr_job->procs[j].status == RUNNING){
            if(waitpid(curr_job->procs[j].pid , &status , WUNTRACED) == -1){
                if(errno == EINTR){
                    kill(-curr_job->pgid , SIGTERM);
                    printf("\nresume: job timed out\n");
                    break;
                }
            }
            if(WIFEXITED(status)){
                // Exit code checks omitted since we don't track pipeline failure here
            }else if(WIFSTOPPED(status)){
                stopped = 1;
                curr_job->is_background = 1;
                for(int k = 0; k < curr_job->procs_count; k++){
                    curr_job->procs[k].status = STOPPED;
                }
                print_job_stopped_or_running(curr_job , 1);
                break;
            }else if(WIFSIGNALED(status)){
                int sig = WTERMSIG(status);
                if(sig == SIGINT){
                    break;
                }
            }
        }
    }

    tcsetpgrp(STDIN_FILENO , getpgid(0));
    if(is_timer) alarm(0);
    if(!stopped){
        int index = -1;
        for(int i = 0 ; i < tracked_job_count ; i++){
            if(&tracked_jobs[i] == curr_job){
                index = i;
                break;
            }
        }

        if(index != -1){
            for(int i = index ;i < tracked_job_count-1 ;i++){
                tracked_jobs[i] = tracked_jobs[i+1];
            }
            tracked_job_count--;
        }
    }
}

int resume_command(Token *command){
    if(command->next == NULL){
        printf("resume: invalid syntax\n");
        return -1;
    }
    
    Token *job_tok = command->next;
    if(job_tok->text == NULL || job_tok->text[0] != '%'){
        printf("resume: invalid syntax\n");
        return -1;
    }
    
    int job_number = atoi(job_tok->text + 1);
    
    // Find job
    JobTrack *target_job = NULL;
    for(int i = 0; i < tracked_job_count; i++){
        if(tracked_jobs[i].job_id == job_number){
            target_job = &tracked_jobs[i];
            break;
        }
    }
    
    if(target_job == NULL){
        printf("resume: no such job\n");
        return -1;
    }
    
    Token *action_tok = job_tok->next;
    if(action_tok == NULL || action_tok->text == NULL){
        printf("resume: invalid syntax\n");
        return -1;
    }
    
    if(strcmp(action_tok->text, "bg") == 0){
        if(action_tok->next != NULL){
            printf("resume: invalid syntax\n");
            return -1;
        }
        resume_background(target_job);
        return 0;
    }else if(strcmp(action_tok->text, "fg") == 0){
        int is_timer = 0;
        int timeout = 0;
        
        Token *flag_tok = action_tok->next;
        if(flag_tok != NULL){
            if(strcmp(flag_tok->text, "--timeout") == 0){
                Token *val_tok = flag_tok->next;
                if(val_tok != NULL && val_tok->text != NULL){
                    timeout = atoi(val_tok->text);
                    if(timeout > 0){
                        is_timer = 1;
                        if(val_tok->next != NULL){
                            printf("resume: invalid syntax\n");
                            return -1;
                        }
                    }else{
                        printf("resume: invalid syntax\n");
                        return -1;
                    }
                }else{
                    printf("resume: invalid syntax\n");
                    return -1;
                }
            }else{
                printf("resume: invalid syntax\n");
                return -1;
            }
        }
        
        resume_foreground(target_job, is_timer, timeout);
        return 0;
    }else{
        printf("resume: invalid syntax\n");
        return -1;
    }
}