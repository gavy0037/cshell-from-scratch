#include <stdio.h>
#include <stdlib.h>
#include "../include/activities.h"
#include "../include/execute.h"

void show_activities() {
    for (int i = 0; i < tracked_job_count; i++) {
        if(tracked_jobs[i].is_background){
            // Find if this group has any active processes
            int has_active = 0;
            for (int j = 0; j < tracked_jobs[i].procs_count; j++) {
                if (tracked_jobs[i].procs[j].status == RUNNING || tracked_jobs[i].procs[j].status == STOPPED) {
                    has_active = 1;
                    break;
                }
            }
            
            if (has_active) {
                printf("[%d] pgid %d\n", tracked_jobs[i].job_id, tracked_jobs[i].pgid);
                for (int j = 0; j < tracked_jobs[i].procs_count; j++) {
                    const char *state_str = NULL;
                    if (tracked_jobs[i].procs[j].status == RUNNING) {
                        state_str = "Running";
                    } else if (tracked_jobs[i].procs[j].status == STOPPED) {
                        state_str = "Stopped";
                    }
                    if(state_str != NULL){
                        printf("  %d %s %s\n", tracked_jobs[i].procs[j].pid, tracked_jobs[i].procs[j].command, state_str);
                    }
                }
            }
        }
    }
}