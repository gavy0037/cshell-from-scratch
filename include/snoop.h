#ifndef SNOOP_H
#define SNOOP_H

#include <sys/types.h>
#include "lexer.h"

#define MAX_SYSCALLS 512

// Structure for syscall mappings
typedef struct {
    long num;
    const char *name;
} SyscallMap;

// Temporary empty table for syscall names. 
// You can populate this array later with the syscalls you want to map.
// Format: { syscall_number, "syscall_name" }
static const SyscallMap syscall_mappings[] = {
    // e.g., { 0, "read" },
    // e.g., { 1, "write" },
    // ADD MORE SYSCALLS HERE LATER
    { -1, NULL } // Terminator
};

// Structure for tracking syscall statistics
typedef struct {
    long syscall_num;
    int count;
    double total_time;
    int first_seen_order;
} SyscallStat;

// Global variables (Defined in snoop.c)
extern SyscallStat stats[MAX_SYSCALLS];
extern int stats_count;
extern int global_order;

int validate_snoop_syntax(Token *cmd);
void snoop_process(Token *cmd);
void snoop_command(Token *cmd);
void snoop_pid(pid_t pid);
void trace_loop(pid_t pid);
const char *syscall_name(long num);
SyscallStat *find_or_create(long num);
int compare_stats(const void *a, const void *b);

#endif