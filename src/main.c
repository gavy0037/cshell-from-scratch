#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/prompt.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/peek.h"
#include "../include/locate.h"
#include "../include/command.h"


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
        Token *head = tokenize(input);
        getcwd(curr_dir , sizeof(curr_dir));

        if((!validate_syntax(head)) == 1){
            printf("cshell:syntax error\n");
            free_tokens(head);
            continue;
        }

        if(head != NULL && head->type == WORD && strcmp(head->text, "hop") == 0) {
            hop(home_dir, prev_dir, curr_dir, head);
        }else if(head != NULL && head->type == WORD && strcmp(head->text , "reveal") == 0){
            reveal(home_dir , prev_dir , curr_dir , head);
        }else if(head != NULL && head->type == WORD && strcmp(head->text , "peek") == 0){
            peek(head , home_dir);
        }else if(head != NULL && head->type == WORD && strcmp(head->text , "locate") == 0){
            locate(head);
        }else if(head != NULL && head->type == WORD && strcmp(head->text, "cd") == 0){
            char *target_dir = home_dir; // Default to home if no argument
            if (head->next != NULL) {
                target_dir = head->next->text;
            }
            if (chdir(target_dir) != 0) {
                perror("cshell");
            }
        }else if(head != NULL && head->type == WORD && strcmp(head->text, "exit") == 0){
            free_tokens(head);
            printf("Exiting...\n");
            exit(0);
        }else{
            // this is a different command , i have to check the current directory for this exec or the path for this directory
            
            process_command(head);
        }
        free_tokens(head);
    }

    return 0;
}