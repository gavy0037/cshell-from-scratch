#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include "../include/snoop.h"
#include "../include/lexer.h"
#include "../include/execute.h"

// Define Global variables
SyscallStat stats[MAX_SYSCALLS];
int stats_count = 0;
int global_order = 0;

int validate_snoop_syntax(Token *cmd) {
    Token *t = cmd->next;
    if (t == NULL) {
        printf("snoop: invalid syntax\n");
        return 0;
    }

    if (strcmp(t->text, "-p") == 0) {
        Token *pid_token = t->next;
        if (pid_token == NULL) {
            printf("snoop: invalid syntax\n");
            return 0;
        }
        
        // Ensure PID is a number
        for (int i = 0; i < (int)strlen(pid_token->text); i++) {
            if (!isdigit(pid_token->text[i])) {
                printf("snoop: invalid syntax\n");
                return 0;
            }
        }
        return 1; // PID mode
    }

    return 2; // COMMAND mode
}

const char *syscall_name(long num) {
    for (int i = 0; syscall_mappings[i].name != NULL; i++) {
        if (syscall_mappings[i].num == num) {
            return syscall_mappings[i].name;
        }
    }
    static char buf[32];
    snprintf(buf, sizeof(buf), "syscall_%ld", num);
    return buf;
}

int compare_stats(const void *a, const void *b) {
    const SyscallStat *sa = (const SyscallStat *)a;
    const SyscallStat *sb = (const SyscallStat *)b;
    if (sb->count != sa->count) return sb->count - sa->count; // descending count
    return sa->first_seen_order - sb->first_seen_order;       // ascending first-seen
}

SyscallStat *find_or_create(long num) {
    for (int i = 0; i < stats_count; i++) {
        if (stats[i].syscall_num == num) return &stats[i];
    }
    stats[stats_count].syscall_num = num;
    stats[stats_count].count = 0;
    stats[stats_count].total_time = 0.0;
    stats[stats_count].first_seen_order = -1;
    return &stats[stats_count++];
}

void trace_loop(pid_t pid) {
    int status;
    int in_syscall = 0; // 0 = expecting entry, 1 = expecting exit
    struct timespec start, end;
    long current_syscall = -1;

    while (1) {
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            break;
        }

        if (WIFSIGNALED(status)) {
            break; // Traced process was killed by a signal
        }

        if (!in_syscall) {
            // Syscall ENTRY
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            current_syscall = regs.orig_rax;
            clock_gettime(CLOCK_MONOTONIC, &start);
            in_syscall = 1;
        } else {
            // Syscall EXIT
            clock_gettime(CLOCK_MONOTONIC, &end);
            double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
            
            SyscallStat *entry = find_or_create(current_syscall);
            entry->count++;
            entry->total_time += elapsed;
            if (entry->first_seen_order == -1) {
                entry->first_seen_order = global_order++;
            }
            
            in_syscall = 0;
        }
    }

    // Cleanup and Summary
    qsort(stats, stats_count, sizeof(SyscallStat), compare_stats);

    // Using exact header format from spec (syscall calls time)
    printf("syscall      calls     time\n");
    for (int i = 0; i < stats_count; i++) {
        printf("%-12s %-8d %.3fs\n", syscall_name(stats[i].syscall_num), stats[i].count, stats[i].total_time);
    }
}

void snoop_pid(pid_t pid) {
    if (kill(pid, 0) == -1 && errno == ESRCH) {
        printf("snoop: no such process\n");
        return;
    }

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        // e.g. EPERM (operation not permitted) if trying to trace a process we don't own
        if (errno == ESRCH) {
            printf("snoop: no such process\n");
        } else {
            perror("ptrace attach failed");
        }
        return;
    }

    int status;
    waitpid(pid, &status, 0);
    trace_loop(pid);
}

void snoop_command(Token *cmd) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // Child
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        char *args[ARGS_MAX];
        Token *t = cmd;
        int i = 0;
        while (t != NULL && i < ARGS_MAX - 1) {
            if (t->type == WORD) {
                args[i++] = t->text;
                t = t->next;
            } else {
                break;
            }
        }
        args[i] = NULL;

        if (execvp(args[0], args) == -1) {
            printf("snoop: command not found\n");
            exit(1);
        }
    } else {
        // Parent
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            // execvp failed and child exited
            return; 
        }

        trace_loop(pid);
    }
}

void snoop_process(Token *cmd) {
    int mode = validate_snoop_syntax(cmd);
    if (mode == 0) return;
    
    // Reset globals for each invocation
    stats_count = 0;
    global_order = 0;
    
    if (mode == 1) { // PID Mode
        pid_t pid = atoi(cmd->next->next->text);
        snoop_pid(pid);
    } else { // COMMAND Mode
        snoop_command(cmd->next);
    }
}