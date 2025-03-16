#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "signal_handles.h"
#include <semaphore.h>

int votante(sem_t *sem1, sem_t *sem2) {
    struct sigaction act_usr1;

    act_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    if (sigaction(SIGUSR1, &act_usr1, NULL) < 0) {
        perror("sigaction SIGUSR1");
        return EXIT_FAILURE;
    }

    sem_wait(sem1);
}