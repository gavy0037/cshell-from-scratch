#include"../include/lexer.h"
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

void append_to_list(Token **head , Token **tail , TokenType type, char *word){
    if (type == WORD && *tail != NULL && (*tail)->type == WORD) {
        char *merged_text = malloc(strlen((*tail)->text) + strlen(word) + 1);

        strcpy(merged_text, (*tail)->text);
        strcat(merged_text, word);
        free((*tail)->text);
        (*tail)->text = merged_text;
        
        return;
    }

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
    } else {
        (*tail)->next = new_node;
        *tail = new_node;
    }
}

Token *tokenize(char *input_string){
    State state = NORMAL;
    int i = 0 ;
    int word_start = -1;
    char current_word[256];
    int word_pointer = 0 ;
    Token *head = NULL, *tail = NULL;


    while(input_string[i] != '\0'){
        if(state == IN_WORD){
            if(input_string[i] == ' ' || input_string[i] == '\t' || input_string[i] == '\n' || input_string[i] == '|' || input_string[i] == '&' || input_string[i] == ';' || input_string[i] == '<' || input_string[i] == '>' || input_string[i] == '\'' || input_string[i] == '"'){
                current_word[word_pointer] = '\0';
                append_to_list(&head, &tail , WORD , current_word);
                word_pointer = 0;
                state = NORMAL;
            } else {
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }
        }else if(state == NORMAL){
            if (input_string[i] == '|') {
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
                    i++; // Skip the second '>' so the loop doesn't process it twice
                }else{
                    append_to_list(&head, &tail, OP_GT, NULL);
                }
            }else if(input_string[i] == '\''){
                state = IN_SQ;
            }else if(input_string[i] == '"'){
                state = IN_DQ;
            }else if(input_string[i] != ' ' && input_string[i] != '\t' && input_string[i] != '\n' && input_string[i] != '\\'){ // - is for flags
                current_word[word_pointer] = input_string[i];
                word_pointer++;
                state = IN_WORD;
            }
            i++;
        }else if(state == IN_SQ){
            if(input_string[i] == '\''){
                state = NORMAL;
                i++;
                current_word[word_pointer] = '\0';
                append_to_list(&head, &tail , WORD,  current_word);
                word_pointer = 0;
            }else{
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }
        }else if(state == IN_DQ){
            if(input_string[i] == '"'){
                state = NORMAL;
                i++;
                current_word[word_pointer] = '\0';
                append_to_list(&head, &tail , WORD,  current_word);
                word_pointer = 0;
            }else{
                current_word[word_pointer] = input_string[i];
                i++;
                word_pointer++;
            }
        }
    }
    if(state == IN_DQ || state == IN_SQ){
        // give error
        printf("cshell: invalid syntax\n");
        // free the list
        return NULL;
    }
    if(state == IN_WORD){
        current_word[word_pointer] = '\0';
        append_to_list(&head , &tail , WORD , current_word);
    }
    return head;
}