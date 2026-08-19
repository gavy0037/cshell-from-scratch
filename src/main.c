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
        }
        free_tokens(head);
    }

    return 0;
}