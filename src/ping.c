#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "../include/ping.h"
#include "../include/execute.h"

int ping_command(Token *command) {
    if (command->next == NULL || command->next->next == NULL || command->next->next->next != NULL) {
        printf("ping: invalid syntax\n");
        return -1;
    }

    Token *target_tok = command->next;
    Token *signal_tok = target_tok->next;

    // Validate signal_number
    char *sig_str = signal_tok->text;
    if (sig_str == NULL || strlen(sig_str) == 0) {
        printf("ping: invalid syntax\n");
        return -1;
    }
    
    // Explicitly check for negative numbers
    if (sig_str[0] == '-') {
        printf("ping: invalid syntax\n");
        return -1;
    }

    // Ensure every character is a digit
    for (size_t i = 0; i < strlen(sig_str); i++) {
        if (!isdigit(sig_str[i])) {
            printf("ping: invalid syntax\n");
            return -1;
        }
    }

    int sig_num = atoi(sig_str);
    int real_sig = sig_num % 64;

    char *target_str = target_tok->text;
    if (target_str == NULL || strlen(target_str) == 0) {
        printf("ping: invalid syntax\n");
        return -1;
    }

    int is_job = 0;
    int target_id = -1;

    if (target_str[0] == '%') {
        is_job = 1;
        
        // Ensure job ID is valid digits after the '%'
        if (strlen(target_str) <= 1) { // Just "%"
            printf("ping: no such process found\n");
            return -1;
        }
        
        for (size_t i = 1; i < strlen(target_str); i++) {
            if (!isdigit(target_str[i])) {
                printf("ping: no such process found\n");
                return -1;
            }
        }
        
        target_id = atoi(target_str + 1);
    } else {
        // Target is a PID
        if (target_str[0] == '-') { // Negative PID Check
            printf("ping: no such process found\n");
            return -1;
        }
        
        for (size_t i = 0; i < strlen(target_str); i++) {
            if (!isdigit(target_str[i])) {
                printf("ping: no such process found\n");
                return -1;
            }
        }
        target_id = atoi(target_str);
    }

    // Lookup target in tracked_jobs
    if (is_job) {
        JobTrack *found_job = NULL;
        for (int i = 0; i < tracked_job_count; i++) {
            if (tracked_jobs[i].job_id == target_id) {
                found_job = &tracked_jobs[i];
                break;
            }
        }
        
        if (found_job == NULL) {
            printf("ping: no such process found\n");
            return -1;
        }
        
        kill(-found_job->pgid, real_sig);
        printf("Sent signal %d to %s\n", sig_num, target_str);
        
    } else {
        int found = 0;
        for (int i = 0; i < tracked_job_count; i++) {
            for (int j = 0; j < tracked_jobs[i].procs_count; j++) {
                if (tracked_jobs[i].procs[j].pid == target_id) {
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }
        
        if (!found) {
            printf("ping: no such process found\n");
            return -1;
        }
        
        kill(target_id, real_sig);
        printf("Sent signal %d to %s\n", sig_num, target_str);
    }

    return 0;
}
