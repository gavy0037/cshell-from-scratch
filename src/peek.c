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
        is_empty = 1;
        int current_line = 1;
        int is_start = 1;
        for(int i = 0; i < read_capacity; i++){
            if(is_line && is_start && isgraph(buffer[i])){ // BUG: If we have a line starting with spaces then i will not print it's line number
                printf("%d ", current_line);
                is_start = 0;
            }
            printf("%c", buffer[i]);

            if(buffer[i] == '\n' && !is_empty){
                current_line++;
                is_empty = 1;
                is_start = 1;
            }
            if(isgraph(buffer[i])){
                is_empty = 0;
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
        perror("peek");
        return;
    }

    if(fstat(fd , &statbuf) == 0 && S_ISDIR(statbuf.st_mode)){
        printf("peek: %s is a directory\n" , file_path);
        close(fd);
        return;
    }

    int current_pos = lseek(fd , 0 ,SEEK_END);

    char line_buffer[4096];
    if(is_reverse){
        int line_count = 1;
        int is_empty = 0;// this is to see if a line is completely empty so that i do not count it in my line count
        while(current_pos > 0){
            int to_read = (current_pos >= CHUNK_SIZE ? CHUNK_SIZE : current_pos);
            
            current_pos-=to_read;
            
            lseek(fd , current_pos , SEEK_SET);
            
            read(fd , buffer , to_read);
            
            for(int i = 0 ; i < to_read ; i++){
                if(buffer[i] == '\n' && !is_empty){
                    line_count++;
                    is_empty = 1;
                }
                if(isgraph(buffer[i])){
                    is_empty = 0 ;
                }
            }
        }
        int current_line = line_count;
        current_pos = lseek(fd, 0, SEEK_END);

        int last_partial_j = 4095;
        int last_partial_is_line_empty = 1;

        while(current_pos > 0){
            int to_read = (current_pos >= CHUNK_SIZE ? CHUNK_SIZE : current_pos);
            current_pos -= to_read;
            lseek(fd, current_pos, SEEK_SET);
            read(fd, buffer, to_read);
            current_pos+=to_read;

            int bytes_printed = 0;
            int i = to_read - 1, j = 4095;

            while(i >= 0){
                int is_line_empty = 1;
                while(i >= 0){
                    if(isgraph(buffer[i])) is_line_empty = 0;
                    line_buffer[j--] = buffer[i--];
                    if(i >= 0 && buffer[i] == '\n') break;
                }

                int is_partial = (i < 0);

                if(!is_partial){
                    if(is_line && !is_line_empty) printf("%d ", current_line);
                    int dec = 0;
                    for(int k = j + 1; k < 4096; k++){
                        printf("%c", line_buffer[k]);
                        bytes_printed++;
                        if(isgraph(line_buffer[k]) && !dec){ current_line--; dec = 1; }
                    }
                    j = 4095;
                } else {
                    last_partial_j = j;
                    last_partial_is_line_empty = is_line_empty;
                }
            }


            current_pos -= bytes_printed;
            lseek(fd, current_pos, SEEK_SET);
        }

        if(last_partial_j < 4095){
            if(is_line && !last_partial_is_line_empty) printf("%d ", current_line);
            for(int k = last_partial_j + 1; k < 4096; k++){
                printf("%c", line_buffer[k]);
            }
        }
    }else{
        int current_line = 1;
        current_pos = lseek(fd , 0 , SEEK_SET);
        int bytes_read = 0;
        while((bytes_read = read(fd , buffer , CHUNK_SIZE)) > 0){
            int bytes_printed = 0;
            int i = 0 ,j = 0 ;
            while(i < bytes_read){
                int is_line_empty = 1;
                while(i < bytes_read){
                    line_buffer[j] = buffer[i];
                    if(isgraph(buffer[i])){
                        is_line_empty = 0;
                    }
                    if(buffer[i] == '\n') break;
                    i++;
                    j++;
                }
                if(is_line && !is_line_empty){
                    printf("%d ",current_line);
                }
                int incremented = 0;
                for(int k =0 ; k < j ; k++){
                    printf("%c",line_buffer[k]);
                    bytes_printed++;
                    if(isgraph(line_buffer[k]) && !incremented){
                        current_line++;
                        incremented = 1;
                    }
                }
                // Print the \n that caused the break
                if(i < bytes_read && buffer[i] == '\n'){
                    printf("\n");
                    bytes_printed++;
                }
                j = 0 ;
                i++;
            }

            current_pos+=bytes_printed;
            lseek(fd , current_pos, SEEK_SET);
        }
        printf("\n");
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