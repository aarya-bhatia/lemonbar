#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define SPACING "   "
#define PYTHON3 "python3"
#define SHELL "/bin/sh"
#define MAX_BUFFER_SIZE 256

#define die(msg)                                                                                                       \
    perror(msg);                                                                                                       \
    exit(1);

enum { UPDATE_PERSIST, UPDATE_INTERVAL, UPDATE_SIGNAL };

struct Module {
    unsigned id;
    int pid;
    int fd[2];
    char read_buf[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    int nread;
    char *command;
    struct Module *next;
    bool escape;
    const char *prefix;
    char *ul_color;
    char *bg_color;
    char *fg_color;
    struct timespec last_updated;
    int type;
    int interval; /* seconds */
};

void setup();
unsigned int num_modules();
struct Module *add_module(const char *command, const char *prefix, int type, int interval);
void free_module(struct Module *module);
void remove_module(struct Module *module);

extern struct Module *modules;
