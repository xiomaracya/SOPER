#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PID 30

#ifndef PRINCIPAL_H
#define PRINCIPAL_H


typedef struct {
    pid_t pid[MAX_PID];
    char vote[MAX_PID];
    int N_PROCS;
    int N_SECS;
    sigset_t mask;
} Network;

void handle_sigint(int sig);

void handle_sigterm(int sig);

void handle_sigalarm(int sig);

void handle_sigusr1(int sig);

void handle_sigusr2(int sig);

#endif