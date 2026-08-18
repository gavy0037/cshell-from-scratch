#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/prompt.h"
#include "../include/hop.h"
#include "../include/reveal.h"

// Helper to convert enum to string for printing
const char* get_token_name(TokenType type) {
    switch (type) {
        case OP_PIPE: return "OP_PIPE";
        case OP_AMP:  return "OP_AMP";
        case OP_SEMI: return "OP_SEMI";
        case OP_LT:   return "OP_LT";
        case OP_GT:   return "OP_GT";
        case OP_GTGT: return "OP_GTGT";
        case WORD:    return "WORD";
        default:      return "UNKNOWN";
    }
}

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
        Token *current = head;

        // Print the token stream
        while (current != NULL) {
            if (current->type == WORD) {
                printf("[%s: '%s'] -> ", get_token_name(current->type), current->text);
            } else {
                printf("[%s] -> ", get_token_name(current->type));
            }
            current = current->next;
        }
        printf("NULL\n");
        if((!validate_syntax(head)) == 1){
            printf("cshell:syntax error\n");
            free_tokens(head);
            continue;
        }

        if(head != NULL && head->type == WORD && strcmp(head->text, "hop") == 0) {
            hop(home_dir, prev_dir, curr_dir, head);
        }else if(head != NULL && head->type == WORD && strcmp(head->text , "reveal") == 0){
            reveal(home_dir , prev_dir , curr_dir , head);
        }
        free_tokens(head);
    }

    return 0;
}