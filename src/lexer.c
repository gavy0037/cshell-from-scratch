#include"../include/lexer.h"
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<stdio.h>

static void free_token_list(Token *head){
    Token *current = head;
    while(current != NULL){
        Token *next = current->next;
        if(current->text != NULL) free(current->text);
        free(current);
        current = next;
    }
}

void append_to_list(Token **head , Token **tail , TokenType type, char *word){
    Token *new_node = (Token*)malloc(sizeof(Token));
    new_node->type = type;
    if(type == WORD){
        new_node->text = malloc(strlen(word) + 1);
        strcpy(new_node->text , word);
    }else new_node->text = NULL;
    
    new_node->next = NULL;
    
    if((*head) == NULL){
        *head = new_node;
        *tail = new_node;
    }else{
        (*tail)->next = new_node;
        *tail = new_node;
    }
}

Token *tokenize(char *input_string){
    State state = NORMAL;
    int i = 0;
    char current_word[256];
    int word_pointer = 0;
    Token *head = NULL, *tail = NULL;

    while(input_string[i] != '\0'){
        if(state == IN_WORD){
            if(input_string[i] == ' ' || input_string[i] == '\t' || input_string[i] == '\n' || input_string[i] == '\r' || input_string[i] == '|' || input_string[i] == '&' || input_string[i] == ';' || input_string[i] == '<' || input_string[i] == '>' || input_string[i] == '\'' || input_string[i] == '"'){
                if(input_string[i] == '\''){
                    state = IN_SQ;
                    i++;
                }else if(input_string[i] == '"'){
                    state = IN_DQ;
                    i++;
                }else{
                    current_word[word_pointer] = '\0';
                    append_to_list(&head, &tail , WORD , current_word);
                    word_pointer = 0;
                    state = NORMAL;
                }
            }else if(input_string[i] == '\\'){
                i++;
                if(input_string[i] == '\0' || input_string[i] == '\n' || input_string[i] == '\r'){
                    free_token_list(head);
                    return NULL;
                }
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }else{
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }
        }else if(state == NORMAL){
            if(input_string[i] == '|'){
                append_to_list(&head, &tail, OP_PIPE, NULL);
            }else if(input_string[i] == '&'){
                append_to_list(&head, &tail, OP_AMP, NULL);
            }else if(input_string[i] == ';'){
                append_to_list(&head, &tail, OP_SEMI, NULL);
            }else if(input_string[i] == '<'){
                append_to_list(&head, &tail, OP_LT, NULL);
            }else if(input_string[i] == '>'){
                if(input_string[i + 1] == '>'){
                    append_to_list(&head, &tail, OP_GTGT, NULL);
                    i++;
                }else{
                    append_to_list(&head, &tail, OP_GT, NULL);
                }
            }else if(input_string[i] == '\\'){
                i++;
                if(input_string[i] == '\0' || input_string[i] == '\n' || input_string[i] == '\r'){
                    free_token_list(head);
                    return NULL;
                }
                current_word[word_pointer++] = input_string[i];
                state = IN_WORD;
            }else if(input_string[i] == '\''){
                state = IN_SQ;
            }else if(input_string[i] == '"'){
                state = IN_DQ;
            }else if(input_string[i] != ' ' && input_string[i] != '\t' && input_string[i] != '\n' && input_string[i] != '\r' && input_string[i] != '\\'){
                current_word[word_pointer] = input_string[i];
                word_pointer++;
                state = IN_WORD;
            }
            i++;
        }else if(state == IN_SQ){
            if(input_string[i] == '\''){
                state = IN_WORD;
                i++;
            }else{
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }
        }else if(state == IN_DQ){
            if(input_string[i] == '\\'){
                i++;
                if(input_string[i] == '\0' || input_string[i] == '\n' || input_string[i] == '\r'){
                    free_token_list(head);
                    return NULL;
                }
                if(input_string[i] != '"' && input_string[i] != '\\'){
                    current_word[word_pointer++] = '\\';
                }
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }else if(input_string[i] == '"'){
                state = IN_WORD;
                i++;
            }else{
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }
        }
    }
    if(state == IN_DQ || state == IN_SQ){
        free_token_list(head);
        return NULL;
    }
    if(state == IN_WORD){
        current_word[word_pointer] = '\0';
        append_to_list(&head , &tail , WORD , current_word);
    }
    return head;
}