#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "signal_handles.h"
#include <semaphore.h>

int votante(sem_t *sem1, sem_t *);