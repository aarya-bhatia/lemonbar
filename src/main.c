#include "log.h"
#include "module.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#define SPACING "   "
#define MAX_EVENTS 10
#define SHELL "/bin/sh"
#define UPDATE_INTERVAL_MS 5000

int epoll_fd;

volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigchld_received = 0;
volatile sig_atomic_t sigusr_received = 0;

void display()
{
    char *display_buffer = calloc(1, (MAX_BUFFER_SIZE + strlen(SPACING)) * num_modules() + 1);
    int index = 0;

    display_buffer[index++] = ' ';

    for (struct Module *module = modules; module; module = module->next) {
        if (strlen(module->buffer) == 0) {
            continue;
        }

        if (module->prefix) {
            memcpy(display_buffer + index, module->prefix, strlen(module->prefix));
            index += strlen(module->prefix);
        }

        memcpy(display_buffer + index, module->buffer, strlen(module->buffer));
        index += strlen(module->buffer);

        memcpy(display_buffer + index, SPACING, strlen(SPACING));
        index += strlen(SPACING);
    }

    display_buffer[index++] = '\n';
    display_buffer[index] = 0;

    write(1, display_buffer, index);
    free(display_buffer);
}

void on_sigchld(int sig)
{
    sigchld_received = 1;
}

void on_sigint(int sig)
{
    sigint_received = 1;
}

void on_sigusr1(int sig)
{
    sigusr_received = 1;
}

void run_once(struct Module *module)
{
    module->fd[0] = 0;
    module->fd[1] = 0;

    if (pipe(module->fd) < 0) {
        die("pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        dup2(module->fd[1], 1);
        close(module->fd[0]);
        close(module->fd[1]);
        log_debug("executing command: %s -c %s", SHELL, module->command);
        if (execlp(SHELL, SHELL, "-c", module->command, NULL) < 0) {
            die("execlp");
        }
    }

    close(module->fd[1]);

    module->nread = 0;

    while (1) {
        int n = read(module->fd[0], module->buffer + module->nread, MAX_BUFFER_SIZE - module->nread - 1);
        if (n <= 0) {
            break;
        }
        module->nread += n;
        module->buffer[module->nread] = 0;
    }

    if (module->nread > 0 && module->buffer[module->nread - 1] == '\n') {
        module->buffer[module->nread - 1] = 0;
    }

    log_debug("received %d bytes from module %d", module->nread, module->id);
    close(module->fd[0]);
    waitpid(pid, NULL, 0);
    clock_gettime(CLOCK_MONOTONIC, &module->last_updated);
}

bool handle_sigusr1()
{
    bool updated = false;
    for (struct Module *module = modules; module; module = module->next) {
        if (module->type == UPDATE_SIGNAL) {
            log_info("Running module %d on SIGUSR1", module->id);
            run_once(module);
            updated = true;
        }
    }

    return updated;
}

void handle_events(struct epoll_event events[MAX_EVENTS], int num_events)
{
    for (int i = 0; i < num_events; ++i) {
        if (events[i].events & EPOLLIN) {
            struct Module *module = events[i].data.ptr;
            int flag = 0;
            for (struct Module *found = modules; found; found = found->next) {
                if (found == module) {
                    flag = 1;
                    break;
                }
            }

            if (flag == 0) {
                log_info("invalid module: %p (id=%d)", module, module->id);
                continue;
            }

            int num_bytes = read(module->fd[0], module->read_buf + module->nread, MAX_BUFFER_SIZE - module->nread - 1);
            if (num_bytes > 0) {
                module->read_buf[num_bytes] = 0;
                char *s;
                if ((s = strstr(module->read_buf, "\n"))) {
                    *s = 0;
                    strcpy(module->buffer, module->read_buf);
                    module->nread = 0;

                    /* if (module->escape) {
                        for (int j = 0; j < num_bytes; j++) {
                            if (module->buffer[j] == '%') {
                                module->buffer[j] = '_';
                            }
                        }
                    } */

                } else {
                    module->nread += num_bytes;
                }
            }
        }
    }
}

void handle_sigchld()
{
    // Reap dead child processes
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            log_warn("Child process %d exited with status %d", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            log_warn("Child process %d terminated by signal %d", pid, WTERMSIG(status));
        }

        for (struct Module *itr = modules; itr; itr = itr->next) {
            if (itr->pid == pid) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, itr->fd[0], NULL);
                // remove_module(itr);
                break;
            }
        }
    }
}

bool check_update_interval()
{
    bool updated = false;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    for (struct Module *itr = modules; itr; itr = itr->next) {
        if (itr->type != UPDATE_INTERVAL) {
            continue;
        }

        double diff = (now.tv_sec - itr->last_updated.tv_sec) + (now.tv_nsec - itr->last_updated.tv_nsec) * 1e-9;
        if (diff - itr->interval > 0) {
            log_info("Running module %d on interval", itr->id);
            log_debug("module %d: time difference: %f", itr->id, diff);
            run_once(itr);
            updated = true;
        }
    }

    return updated;
}

int main(int argc, char *argv[])
{
    signal(SIGCHLD, on_sigchld);
    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);
    signal(SIGUSR1, on_sigusr1);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        die("epoll_create1");
    }

    setup();

    int num = num_modules();
    if (num == 0) {
        fputs("no modules are avilable", stderr);
        exit(0);
    }

    log_debug("num modules added: %u", num_modules());

    while (!sigint_received) {
        struct epoll_event events[MAX_EVENTS];
        memset(events, 0, sizeof events);
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, UPDATE_INTERVAL_MS);
        if (num_events == -1) {
            perror("epoll_wait");
            if (errno != EINTR) {
                log_fatal("errno=%d", errno);
                exit(1);
            }
        }

        if (num_events > 0) {
            handle_events(events, num_events);
            display();
        }

        if (sigusr_received) {
            if (handle_sigusr1()) {
                display();
            }
            sigusr_received = 0;
        }

        if (sigchld_received) {
            handle_sigchld();
            sigchld_received = 0;
        }

        if (check_update_interval()) {
            display();
        }

        sleep(1);
    }

    for (struct Module *module = modules; module; module = module->next) {
        if (module->type == UPDATE_PERSIST) {
            log_warn("sending SIGTERM to module %d", module->id);
            kill(module->pid, SIGTERM);
        }
    }

    while (modules) {
        struct Module *tmp = modules->next;
        if (modules->type == UPDATE_PERSIST) {
            waitpid(modules->pid, NULL, 0);
        }
        log_warn("freeing module %d", modules->id);
        free_module(modules);
        modules = tmp;
    }

    log_fatal("goodbye!");
    close(epoll_fd);
    return 0;
}
