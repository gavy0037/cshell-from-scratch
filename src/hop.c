#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include"../include/lexer.h"
#include"../include/hop.h"

#define MAX_NODES 64

static void free_frecency_list(FrecencyNode *head) {
    while (head != NULL) {
        FrecencyNode *temp = head;
        head = head->next;
        free(temp->path);
        free(temp);
    }
}

FrecencyNode *load_frecency_list(char *shell_home) {
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/.cshell_frecency", shell_home);

    FILE *f = fopen(file_path, "r");
    if (!f) {
        return NULL;
    }

    FrecencyNode *head = NULL;
    FrecencyNode *tail = NULL;
    
    char *path_line = NULL;
    size_t path_len = 0;
    char *freq_line = NULL;
    size_t freq_len = 0;

    int node_count = 0;

    while (node_count < MAX_NODES && getline(&path_line, &path_len, f) != -1) {
        if (getline(&freq_line, &freq_len, f) == -1) {
            break;
        }

        int plen = strlen(path_line);
        if (plen > 0 && path_line[plen - 1] == '\n') {
            path_line[plen - 1] = '\0';
        }
        
        int freq = atoi(freq_line);

        FrecencyNode *node = malloc(sizeof(FrecencyNode));
        if (!node) break;
        node->path = strdup(path_line);
        node->frequency = freq;
        node->next = NULL;

        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        node_count++;
    }

    fclose(f);
    
    return head;
}

void save_frecency_list(char *shell_home, FrecencyNode *head) {
    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/.cshell_frecency", shell_home);

    FILE *f = fopen(file_path, "w");
    if (!f) return;

    FrecencyNode *curr = head;
    while (curr) {
        fprintf(f, "%s\n%d\n", curr->path, curr->frequency);
        curr = curr->next;
    }

    fclose(f);
}

void update_frecency(char *shell_home , char *abs_path) {
    FrecencyNode *head = load_frecency_list(shell_home);
    FrecencyNode *curr = head;
    FrecencyNode *prev = NULL;

    while (curr) {
        if (strcmp(curr->path, abs_path) == 0) {
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    if (curr) {
        curr->frequency++;
        if (prev) {
            prev->next = curr->next;
            curr->next = head;
            head = curr;
        }
    } else {
        FrecencyNode *new_node = malloc(sizeof(FrecencyNode));
        if (new_node) {
            new_node->path = strdup(abs_path);
            new_node->frequency = 1;
            new_node->next = head;
            head = new_node;
        }
    }

    curr = head;
    FrecencyNode *tail = NULL;
    int count = 0;
    while (curr) {
        count++;
        if (count == MAX_NODES) {
            tail = curr;
            break;
        }
        curr = curr->next;
    }

    if (tail && tail->next) {
        free_frecency_list(tail->next);
        tail->next = NULL;
    }

    save_frecency_list(shell_home, head);
    free_frecency_list(head);
}

int compare_matches(const void *a, const void *b) {
    const FrecencyMatch *ma = (const FrecencyMatch *)a;
    const FrecencyMatch *mb = (const FrecencyMatch *)b;

    if (ma->score > mb->score) return -1;
    if (ma->score < mb->score) return 1;
    
    return strcmp(ma->node->path, mb->node->path);
}

int fallback_hop(char *shell_home, char *target) {
    FrecencyNode *head = load_frecency_list(shell_home);
    if (!head) {
        fprintf(stderr, "hop: No matching directory found\n");
        return -1;
    }

    int total_nodes = 0;
    FrecencyNode *curr = head;
    while (curr) {
        total_nodes++;
        curr = curr->next;
    }

    FrecencyMatch *matches = malloc(sizeof(FrecencyMatch) * total_nodes);

    int match_count = 0;
    curr = head;
    int index = 0;
    
    while (curr) {
        if (strstr(curr->path, target) != NULL) {
            matches[match_count].node = curr;
            matches[match_count].score = curr->frequency * (total_nodes - index);
            match_count++;
        }
        curr = curr->next;
        index++;
    }

    if (match_count == 0) {
        fprintf(stderr, "hop: No matching directory found\n");
        free(matches);
        free_frecency_list(head);
        return -1;
    }

    qsort(matches, match_count, sizeof(FrecencyMatch), compare_matches);

    int success = 0;
    for (int i = 0; i < match_count; i++) {
        if (chdir(matches[i].node->path) == 0) {
            success = 1;
            break;
        }
    }

    if (!success) {
        fprintf(stderr, "hop: No matching directory found\n");
    }

    free(matches);
    free_frecency_list(head);

    return success ? 0 : -1;
}
char *resolve_path(char *path , char *prev_dir , char *home_dir , char *curr_dir){
    if(strlen(path) == 0){
        return curr_dir;
    }else if(path[0] == '~'){
        char temp[4096];
        strcpy(temp , path);
        strcpy(path , home_dir);
        strcat(path , temp);
        return path;
    }else if(path[0] == '-' && path[1] == '\0'){
        return prev_dir;
    }else{
        return path;
    }
}

void hop(char *home_dir , char *prev_dir, char *curr_dir, Token *head){
    Token *t = head->next;
    char abs_path[4096];

    if(t == NULL){
        getcwd(prev_dir , 4096);
        if (chdir(home_dir) == 0) {
            if (getcwd(abs_path, sizeof(abs_path))) {
                update_frecency(home_dir, abs_path);
            }
        }
        return;
    }
    
    if(t->text[0] == '-' && strlen(prev_dir) == 0){
        getcwd(prev_dir , 4096);
        return;
    }
    char buffer[4096];
    while(t != NULL){
        getcwd(buffer , 4096);
        strcpy(curr_dir, resolve_path(t->text , prev_dir , home_dir , curr_dir));
        
        if(chdir(curr_dir) == 0){
            strcpy(prev_dir , buffer);
            if (getcwd(abs_path, sizeof(abs_path))) {
                update_frecency(home_dir, abs_path);
            }
        } else {
            // Native chdir failed, invoke fallback hook
            if (fallback_hop(home_dir, t->text) == 0) {
                strcpy(prev_dir, buffer);
                if (getcwd(abs_path, sizeof(abs_path))) {
                    update_frecency(home_dir, abs_path);
                }
            }
        }
        t = t->next;
    }
}