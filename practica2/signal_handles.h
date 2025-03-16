#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_PID 30

void handle_sigint(int sig);

void handle_sigterm(int sig);

void handle_sigalarm(int sig);

void handle_sigusr1(int sig);

void handle_sigusr2(int sig);