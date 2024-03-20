#include "module.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

struct Module *modules = NULL;
extern int epoll_fd;

unsigned int num_modules()
{
    int num_modules = 0;
    for (struct Module *module = modules; module; module = module->next) {
        num_modules++;
    }
    return num_modules;
}

struct Module *add_module(const char *command, const char *prefix, int type, int interval)
{
    assert(command);

    struct Module *module = calloc(1, sizeof *module);
    if (!module) {
        die("calloc");
    }

    memset(module->buffer, 0, sizeof(module->buffer));
    memset(module->buffer, 0, sizeof(module->read_buf));

    module->command = strdup(command);
    module->id = num_modules();
    module->prefix = prefix;
    module->type = type;
    module->interval = interval;

    if (pipe(module->fd) < 0) {
        die("pipe");
    }

    int pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        dup2(module->fd[1], 1);
        close(module->fd[0]);
        close(module->fd[1]);
        execlp(SHELL, SHELL, "-c", command, NULL);
        exit(1);
    }

    fprintf(stderr, "started module %d: %s\n", module->id, command);
    close(module->fd[1]);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = module;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, module->fd[0], &ev) < 0) {
        die("epoll_ctl");
    }

    if (!modules) {
        modules = module;
    } else {
        struct Module *tail = modules;
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = module;
    }

    module->pid = pid;
    clock_gettime(CLOCK_MONOTONIC, &module->last_updated);

    return module;
}

void remove_module(struct Module *module)
{
    if (modules == module) {
        modules = module->next;
    } else {
        struct Module *itr = modules;
        while (itr && itr->next != module) {
            itr = itr->next;
        }
        if (itr->next == module) {
            itr->next = module->next;
        }
    }

    fprintf(stderr, "module %u removed...\n", module->id);
    free_module(module);
}

void free_module(struct Module *module)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, module->fd[0], NULL);
    free(module->command);
    close(module->fd[0]);
    free(module);
}
