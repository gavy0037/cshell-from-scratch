#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include <fcntl.h>
#include"../include/lexer.h"
#include"../include/peek.h"
#include"../include/redirect.h"

void handle_input_redirection(Token *head){
    // there would be a file name following a <
    
    Token *t = head;

    while(t != NULL){
        if(t->type == OP_LT){
            
            int fd = open(t->next->text , O_RDONLY);

            if(fd == -1){
                printf("cshell: no such file or directory\n");
                close(fd);
                exit(1);
            }
            close(fd);
            t = t->next->next;
        }else t = t->next;
    }

    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if(pid == 0){
        close(pipefd[0]);
        char *file_name;
        t = head;
        while(t != NULL){
            if(t->type == OP_LT){
                file_name = t->next->text;
                int fd = open(file_name , O_RDONLY);
                int bytes_read;
                char buffer[4096];
                while((bytes_read = read(fd , buffer , CHUNK_SIZE)) > 0){
                    write(pipefd[1] , buffer, bytes_read);
                }
                close(fd);
                t = t->next->next;
            }else t = t->next;
        }
        exit(0);
    }else{
        // parent
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
    }
}


void handle_output_redirection(Token *head){
    Token *t = head;
    int fds[256];
    int idx = 0;
    int pipefd[2];
    
    
    while(t != NULL){
        if(t->type == OP_GT || t->type == OP_GTGT){
            int flags = O_WRONLY | O_CREAT | ((t->type == OP_GT) ? O_TRUNC : O_APPEND);
            
            int fd = open(t->next->text , flags , 0644);

            if(fd == -1){
                printf("cshell: unable to create file for writing\n");
                for(int i = 0 ; i < idx ; i++){
                    close(fds[i]);
                }
                exit(1);
            }
            t = t->next->next;
            fds[idx++] = fd;
        }else t = t->next;
    }
    
    pipe(pipefd);
    pid_t pid = fork();
    if(pid == 0){
        close(pipefd[1]); // close write end

        int bytes_read;
        char buffer[4096];
        while((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0){
            for(int i = 0; i < idx; i++){
                write(fds[i], buffer, bytes_read);
            }
        }

        for(int i = 0; i < idx; i++) close(fds[i]);
        close(pipefd[0]);
        exit(0);

    } else {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        for(int i = 0 ; i < idx ; i++){
            close(fds[i]);// because this fds are shared with both the parent and the child so i must free them in both o
        }
    }
}