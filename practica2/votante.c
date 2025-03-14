#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"

int votante() {
    struct sigaction act_usr1;

    act_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    sigaction(SIGUSR1)
}