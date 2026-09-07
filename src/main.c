#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>
#include <signal.h>
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


int main() {

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGCHLD , &sa , NULL) == -1){
        perror("sigaction");
        exit(1);
    }


    getcwd(home_dir , sizeof(home_dir));
    strcpy(curr_dir , home_dir);
    while (1) {
        display_prompt(home_dir);
        fflush(stdout);
        char input[4096];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nExiting...\n");
            break;
        }

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