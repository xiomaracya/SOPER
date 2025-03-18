#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#include <strings.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

int votante(sem_t *sem1, sem_t *sem2, Network *network, FILE *f) {
    struct sigaction act_usr1;
    struct sigaction act_usr2;

    char voto;
    int i, flag=0, pid;
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

    if (sigaction(SIGUSR2, &act_usr2, NULL) < 0) {
        perror("sigaction SIGUSR2");
        return EXIT_FAILURE;
    }

    // SEMÁFORO = 1 -> CANDIDATO
    if(sem_trywait(sem1) == 0) {
        printf("Candidato");
        for (int i = 0; i < network->N_PROCS; i++) {
            if(network->pid[i] != getpid()) {
                kill(network->pid[i], SIGUSR2);
            }
        }

        f = fopen("PIDS", "r");
        while (flag==0) {
            flag = 1;
            for(i=0; i<network->N_PROCS; i++) {
                fscanf(f, "Proceso %d vota %c\n", &pid, &voto);
                network->vote[i] = voto;
                if (voto != 'Y' && voto != 'N') {
                    flag = 0;
                    break;
                }
            }
            if (flag == 1){
                break;
            }
            sleep(0.001);
        }

        printf("Candidate %d => [", getpid());
        for (int i = 0; i < network->N_PROCS; i++) {
            printf(" %c ", network->vote[i]);
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

        sleep(2);

        return EXIT_SUCCESS;
    /// SEMAFORO = 0 -> VOTANTES
    } else {
        printf("Votantes\n");
        sem_wait(sem2);

        srand(getpid());
        if(rand() % 2==0) {
            voto = 'N';
        } else {
            voto = 'Y';
        }

        FILE *temp_f = fopen("PIDS", "a+");  // Abrir archivo para añadir votos en modo sobreescritura
        if (temp_f == NULL) {
            perror("Error abriendo el archivo PIDS");
            return EXIT_FAILURE;
        }

        fprintf(temp_f, "Proceso %d vota %c\n", getpid(), voto);
        fclose(temp_f);

        sem_post(sem2);
        return EXIT_SUCCESS;
    }
}