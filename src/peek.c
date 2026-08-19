#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<ctype.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>

#include"../include/peek.h"
#include"../include/lexer.h"
#include"../include/hop.h"

void read_stdin(int is_reverse , int is_line){
    int fd = STDIN_FILENO;
    int capacity = 4096;
    int read_capacity = 0;
    char *buffer = malloc(capacity);
    int bytes_read = 0;
    while((bytes_read = read(fd , buffer+read_capacity , capacity-read_capacity)) > 0){
        read_capacity+=bytes_read;

        if(read_capacity == capacity){
            capacity = capacity*2;
            buffer = realloc(buffer , capacity);
        }
    }

    // now i have read the bytes
    int line_count = 1;
    int is_empty = 0;// this is to see if a line is completely empty so that i do not count it in my line count
    for(int i = 0 ; i < read_capacity ; i++){
        if(buffer[i] == '\n' && !is_empty){
            line_count++;
            is_empty = 1;
        }
        if(isgraph(buffer[i])){
            is_empty = 0 ;
        }
    }

    
    if(is_reverse){
        int current_line = line_count;
        char line_buffer[4096];
        int i = read_capacity - 1;
        while(i >= 0){
            int j = 4095;
            int is_line_empty = 1;
            while(i >= 0){
                if(isgraph(buffer[i])){
                    is_line_empty = 0;
                }
                line_buffer[j--] = buffer[i--];
                if(i >= 0 && buffer[i] == '\n') break;
            }
            
            if(is_line && !is_line_empty){
                printf("%d ", current_line);
            }
            
            int dec = 0;
            for(int k = j + 1; k < 4096; k++){
                printf("%c", line_buffer[k]);
                if(isgraph(line_buffer[k]) && !dec){
                    current_line--;
                    dec = 1;
                }
            }
        }
    }else{
        int current_line = 1;
        int i = 0;
        while(i < read_capacity){
            // find the end of this line and check for graph chars
            int start = i;
            int has_graph = 0;
            while(i < read_capacity && buffer[i] != '\n'){
                if(isgraph(buffer[i])) has_graph = 1;
                i++;
            }
            if(i < read_capacity) i++;

            if(is_line && has_graph){
                printf("%d ", current_line++);
            }
            for(int k = start; k < i; k++){
                printf("%c", buffer[k]);
            }
        }
    }
    fflush(stdout);
}

void read_file(int is_reverse , int is_line , char *file_path , char *home_dir){
    char *resolved_path = resolve_path(file_path , "" , home_dir , "");
    char buffer[CHUNK_SIZE];// for reading in chunks
    struct stat statbuf;
    int fd = open(resolved_path , O_RDONLY);

    if(fd < 0){
        printf("peek: no such file or directory\n");
        return;
    }

    if(fstat(fd , &statbuf) == 0 && S_ISDIR(statbuf.st_mode)){
        printf("peek: is a directory\n");
        close(fd);
        return;
    }

    int current_pos = lseek(fd , 0 ,SEEK_END);

    if(is_reverse){
        int line_count = 0;
        int has_content = 0;
        while(current_pos > 0){
            int to_read = (current_pos >= CHUNK_SIZE ? CHUNK_SIZE : current_pos);
            current_pos -= to_read;
            lseek(fd, current_pos, SEEK_SET);
            read(fd, buffer, to_read);
            for(int i = 0; i < to_read; i++){
                if(isgraph(buffer[i])){
                    has_content = 1;
                }
                if(buffer[i] == '\n'){
                    if(has_content) line_count++;
                    has_content = 0;
                }
            }
        }
        if(has_content) line_count++; // last line without trailing \n

        int file_size = lseek(fd, 0, SEEK_END);
        current_pos = file_size;

        if(current_pos > 0){
            char last;
            lseek(fd, current_pos - 1, SEEK_SET);
            if(read(fd, &last, 1) == 1 && last == '\n'){ // to remove the last \n
                current_pos--;
            }
        }

        char *leftover = NULL;
        int leftover_len = 0;
        int current_line = line_count;

        while(current_pos > 0){
            int to_read = (current_pos >= CHUNK_SIZE ? CHUNK_SIZE : current_pos);
            current_pos -= to_read;
            lseek(fd, current_pos, SEEK_SET);
            read(fd, buffer, to_read);

            int end = to_read; // right boundary within buffer

            for(int i = to_read - 1; i >= 0; i--){
                if(buffer[i] == '\n'){
                    // Line content: buffer[i+1 .. end-1] + leftover
                    int non_empty = 0;
                    for(int c = i + 1; c < end && !non_empty; c++)
                        if(isgraph(buffer[c])) non_empty = 1;
                    for(int c = 0; c < leftover_len && !non_empty; c++)
                        if(isgraph(leftover[c])) non_empty = 1;

                    if(is_line && non_empty) printf("%d ", current_line);
                    if(non_empty) current_line--;

                    for(int c = i + 1; c < end; c++) printf("%c",buffer[c]);
                    for(int c = 0; c < leftover_len; c++) printf("%c" ,leftover[c]);
                    printf("\n");

                    leftover = NULL;
                    leftover_len = 0;
                    end = i; // consumed the \n, move boundary left
                }
            }
            if(end > 0){
                char *new_lo = malloc(end + leftover_len);
                memcpy(new_lo, buffer, end);
                if(leftover_len > 0) memcpy(new_lo + end, leftover, leftover_len); // could have used strncpy but it terminates on seeing a /0 even if the string is not completely copied of the specified length
                free(leftover);
                leftover = new_lo;
                leftover_len = end + leftover_len;
            }
        }
        if(leftover_len > 0){
            int non_empty = 0;
            for(int c = 0; c < leftover_len; c++)
                if(isgraph(leftover[c])){
                    non_empty = 1; 
                    break;
                }
            if(is_line && non_empty)
                printf("%d ", current_line);
            for(int c = 0; c < leftover_len; c++)
                printf("%c",leftover[c]);
            printf("\n");
            free(leftover);
            leftover = NULL;
        }
    }else{
        int current_line = 1;
        lseek(fd, 0, SEEK_SET);
        int bytes_read = 0;

        char line_buf[4096];
        int line_len = 0;
        int has_graph = 0;

        while((bytes_read = read(fd, buffer, CHUNK_SIZE)) > 0){
            for(int i = 0; i < bytes_read; i++){
                if(isgraph(buffer[i])) has_graph = 1;

                line_buf[line_len++] = buffer[i];

                if(buffer[i] == '\n'){
                    if(is_line && has_graph){
                        printf("%d ", current_line++);
                    }
                    for(int k = 0; k < line_len; k++){
                        printf("%c", line_buf[k]);
                    }
                    line_len = 0;
                    has_graph = 0;
                }
            }
        }
        // handle last line if file doesn't end with '\n'
        if(line_len > 0){
            if(is_line && has_graph){
                printf("%d ", current_line++);
            }
            for(int k = 0; k < line_len; k++){
                printf("%c", line_buf[k]);
            }
        }
    }
    fflush(stdout);
    close(fd);
}

void peek(Token *head , char *home_dir){
    Token *t = head->next;
    int is_reverse = 0 , is_line = 0;
    while(t != NULL && t->type == WORD && t->text[0] == '-' && strlen(t->text) > 1){
        for(int i = 1 ; i < (int)strlen(t->text) ; i++){
            if(t->text[i] == 'n'){
                is_line = 1;
            }else if(t->text[i] == 'r'){
                is_reverse = 1;
            }else{
                printf("peek: invalid syntax\n");
                return;
            }
        }
        t = t->next;
    }
    if(t == NULL){
        read_stdin(is_reverse , is_line);
    }
    while(t != NULL){
        if(strcmp(t->text , "-") == 0){
            // use stdin for input
            read_stdin(is_reverse , is_line);
        }else{
            // this is standard file input
            read_file(is_reverse , is_line , t->text , home_dir);
        }
        t = t->next;
    }
}