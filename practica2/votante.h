#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#include <semaphore.h>
#include <errno.h>

#ifndef VOTANTE_H
#define VOTANTE_H

int votante(int fd, sem_t *sem1, sem_t *sem2, sem_t *sem3, int nProc, sigset_t mask);

#endif