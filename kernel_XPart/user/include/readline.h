#ifndef READLINE_H
#define READLINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 128
#define MAX_HISTORY 32
#define MAX_AUTO_COMPLETE 16

typedef struct {
    char *lines[MAX_HISTORY];
    int count;
    int current;
} history; 

void readline(char* line, int* length);

#endif