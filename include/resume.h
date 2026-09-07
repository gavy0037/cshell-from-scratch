#ifndef RESUME_H
#define RESUME_H

#include<stdio.h>
#include<stdlib.h>

#include "execute.h"
#include "lexer.h"

void resume_background(JobTrack *curr_job);
void resume_foreground(JobTrack *curr_job , int is_timer , int timeout);
int resume_command(Token *command);

#endif