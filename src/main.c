#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>
#include <signal.h>
#include <errno.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/prompt.h"
#include "../include/execute.h"


// Helper to free the linked list to prevent memory leaks
void free_tokens(Token *head) {
    Token *current = head;
    while (current != NULL) {
        Token *next = current->next;
        if (current->text != NULL) {
            free(current->text);
        }
        free(current);
        current = next;
    }
}


char home_dir[4096] , prev_dir[4096] = "", curr_dir[4096];

void dummy_sigalrm_handler(int sig) {
    // Dummy handler to interrupt waitpid without restarting it
}

static volatile sig_atomic_t sigint_flag = 0;
void sigint_handler(int sig) {
    sigint_flag = 1;
}

int main() {

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD , &sa , NULL) == -1){
        perror("sigaction");
        exit(1);
    }

    struct sigaction sa_alrm;
    sa_alrm.sa_handler = dummy_sigalrm_handler;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = 0; // NO SA_RESTART so that we do not restart waitpid in resuming fg process , if we did , then that would consume the interuppt and we will be stuck there forever
    sigaction(SIGALRM, &sa_alrm, NULL);

    /* SIGINT: catch it so Ctrl+C at the prompt re-displays the prompt instead of exiting */
    struct sigaction sa_int;
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; /* No SA_RESTART — fgets() must be interrupted */
    sigaction(SIGINT, &sa_int, NULL);

    signal(SIGTSTP , SIG_IGN);
    signal(SIGTTOU , SIG_IGN);

    getcwd(home_dir , sizeof(home_dir));
    strcpy(curr_dir , home_dir);
    int consecutive_eof = 0;
    while (1) {
        print_completed_jobs();
        display_prompt(home_dir);
        fflush(stdout);
        char input[4096];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            if(sigint_flag){
                sigint_flag = 0;
                printf("\n");
                clearerr(stdin);
                continue;
            }
            if(errno == EINTR){
                clearerr(stdin);
                continue;
            }
            if (feof(stdin)) {
                clearerr(stdin); // Clear EOF state so we can read again if we don't exit
                
                if (consecutive_eof) {
                    kill_all_jobs();
                    printf("\nExiting...\n");
                    break;
                }
                
                if (has_stopped_jobs()) {
                    printf("\ncshell: there are stopped jobs\n");
                    consecutive_eof = 1;
                    continue;
                }
                
                // No stopped jobs
                kill_all_jobs();
                printf("\nExiting...\n");
                break;
            } else {
                clearerr(stdin);
                continue;
            }
        }
        
        /* Clear SIGINT flag in case it fired between fgets returning and here */
        if(sigint_flag){
            sigint_flag = 0;
            printf("\n");
            continue;
        }

        consecutive_eof = 0;

        int is_empty = 1;
        for(int i =0 ; input[i] != '\0' ; i++){
            if(input[i] != ' ' && input[i] != '\t' && input[i] != '\n' && input[i] != '\r'){
                is_empty = 0;
                break;
            }
        }
        if(is_empty) continue;
        Token *head = tokenize(input);
        getcwd(curr_dir , sizeof(curr_dir));

        if((!validate_syntax(head)) == 1){
            printf("cshell: invalid syntax\n");
            free_tokens(head);
            continue;
        }
        execute(head , home_dir , prev_dir , curr_dir);
        
        free_tokens(head);
    }

    return 0;
}