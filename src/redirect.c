#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include"../include/lexer.h"
#include"../include/peek.h"
#include"../include/redirect.h"

void handle_input_redirection(Token *head){
    Token *t = head;
    int has_input = 0;

    while(t != NULL){
        if(t->type == OP_LT && t->next != NULL){
            int fd = open(t->next->text, O_RDONLY);
            if(fd == -1){
                printf("cshell: no such file or directory\n");
                exit(1);
            }
            close(fd);
            has_input = 1;
            t = t->next->next;
        }else{
            t = t->next;
        }
    }

    if(!has_input) return;

    char temp_template[] = "/tmp/.cshell_in_XXXXXX";
    int temp_fd = mkstemp(temp_template);
    if(temp_fd == -1) exit(1);
    unlink(temp_template); // unlink so file deletes automatically when closed

    t = head;
    char buffer[4096];
    while(t != NULL){
        if(t->type == OP_LT && t->next != NULL){
            int fd = open(t->next->text, O_RDONLY);
            if(fd != -1){
                int bytes;
                while((bytes = read(fd, buffer, sizeof(buffer))) > 0){
                    write(temp_fd, buffer, bytes);
                }
                close(fd);
            }
            t = t->next->next;
        }else{
            t = t->next;
        }
    }

    lseek(temp_fd, 0, SEEK_SET); // rewind temp file to beginning
    dup2(temp_fd, STDIN_FILENO); // redirect stdin to temp file
    close(temp_fd);
}


void handle_output_redirection(Token *head){
    Token *t = head;
    int fds[256];
    int count = 0;

    while(t != NULL){
        if((t->type == OP_GT || t->type == OP_GTGT) && t->next != NULL){
            int flags = O_WRONLY | O_CREAT | ((t->type == OP_GT) ? O_TRUNC : O_APPEND);
            int fd = open(t->next->text, flags, 0644);
            if(fd == -1){
                printf("cshell: unable to create file for writing\n");
                for(int i = 0 ; i < count ; i++) close(fds[i]);
                exit(1);
            }
            fds[count++] = fd;
            t = t->next->next;
        }else{
            t = t->next;
        }
    }

    if(count == 0) return;

    if(count == 1){
        dup2(fds[0], STDOUT_FILENO); // redirect stdout directly to file
        close(fds[0]);
        return;
    }

    char temp_template[] = "/tmp/.cshell_out_XXXXXX";
    int temp_fd = mkstemp(temp_template);
    if(temp_fd == -1) exit(1);
    unlink(temp_template); // unlink so file deletes automatically on close

    pid_t sub_pid = fork();
    if(sub_pid == 0){
        // child process to execute the command into the temp file
        dup2(temp_fd, STDOUT_FILENO);
        close(temp_fd);
        for(int i = 0 ; i < count ; i++) close(fds[i]);
        return;
    }else{
        // parent waits for sub process to finish writing to temp file
        int sub_status;
        waitpid(sub_pid, &sub_status, 0);

        lseek(temp_fd, 0, SEEK_SET); // rewind temp file to beginning
        char buffer[4096];
        int bytes;
        while((bytes = read(temp_fd, buffer, sizeof(buffer))) > 0){
            for(int i = 0 ; i < count ; i++){
                write(fds[i], buffer, bytes);
            }
        }
        for(int i = 0 ; i < count ; i++) close(fds[i]);
        close(temp_fd);
        if(WIFEXITED(sub_status)){
            exit(WEXITSTATUS(sub_status));
        }else if(WIFSIGNALED(sub_status)){
            exit(128 + WTERMSIG(sub_status));
        }else{
            exit(0);
        }
    }
}