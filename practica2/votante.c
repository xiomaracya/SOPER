#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#include <semaphore.h>
#include <time.h>
#include <errno.h>

int votante(sem_t *sem1, sem_t *sem2, Network *network, FILE *f) {
    struct sigaction act_usr1;
    struct sigaction act_usr2;

    char palabra [4];
    pid_t pid=0;
    char voto;
    int i, flag=0;
    int yes = 0, no = 0;

    act_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    act_usr2.sa_handler = handle_sigusr2;
    sigemptyset(&(act_usr2.sa_mask));
    act_usr2.sa_flags = 0;

    if (sigaction(SIGUSR1, &act_usr1, NULL) < 0) {
        perror("sigaction SIGUSR1");
        return EXIT_FAILURE;
    }

    if(sem_trywait(sem1) == 0) {
        printf("OK");
        for (int i = 0; i < network->N_PROCS; i++) {
            if(network->pid[i] != getpid()) {
                kill(network->pid[i], SIGUSR2);
            }
        }

        while (flag==0) {
            for(i=0; i<network->N_PROCS; i++) {
                if (network->pid[i] == 'Y' || network->pid[i] == 'N') {
                    flag=1;
                } else {
                    flag=0;
                    break;
                }
            }
            sleep(0.001);
        }

        for (i=0; i<network->N_PROCS; i++) {
            fscanf(f, "%s %d", palabra, &pid);
            fprintf(f, "VOTO %c", network->vote[i]);
        }

        printf("Candidate %d => [", getpid());
        for (int i = 0; i < network->N_PROCS; i++) {
            printf("%c", network->vote[i]);
        }
        printf("] => ");
        for (i=0; i<network->N_PROCS; i++) {
            if (network->vote[i] == 'Y') {
                yes++;
            } else {
                no++;
            }
        }

        if(yes > no) {
            printf("Accepted\n");
        } else {
            printf("Rejected\n");
        }
        fflush(stdout);

        sleep(0.25);
        return EXIT_SUCCESS;
    } else {
        if (sigaction(SIGUSR2, &act_usr2, NULL) < 0) {
            perror("sigaction SIGUSR2");
            return EXIT_FAILURE;
        }

        sem_wait(sem2);

        srand(time(NULL));
        if(rand() % 2==0) {
            voto = 'N';
        } else {
            voto = 'Y';
        }

        for (i=0; i<network->N_PROCS; i++) {
            if (network->pid[i] == getpid()) {
                break;
            }
        }

        network->vote[i] = voto;

        sem_post(sem2);
        return EXIT_SUCCESS;
    }


}