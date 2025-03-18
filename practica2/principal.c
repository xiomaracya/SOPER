#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#include <semaphore.h>
#include "votante.h"
#include <errno.h>

void handle_sigint(int sig) {
    
    printf("Finishing by signal\n");

    exit(EXIT_SUCCESS);

}

void handle_sigterm(int sig) {

    exit(EXIT_SUCCESS);

}

void handle_sigalarm(int sig) {

    printf("Finishing by alarm\n");
    exit(EXIT_SUCCESS);

}

void handle_sigusr1(int sig) {

}

void handle_sigusr2(int sig) {


}

int main (int argc, char *argv[]){

    sem_t *sem1 = NULL;
    sem_t *sem2 = NULL;

    // INICIALIZAR HANDLES
    FILE *f;
    int i, sig;
    pid_t pid;
    struct sigaction act_int, act_usr1, act_usr2, act_term, act_alarm;
    Network network;
    sigset_t mask;

    if(argc==3) {
        network.N_PROCS = atoi(argv[1]);
        network.N_SECS = atoi(argv[2]);
    }

    // CONTROL DE PARÁMETROS
    if (argc != 3 || network.N_PROCS < 1 || network.N_SECS < 1) {
        while(network.N_PROCS < 1 || network.N_SECS < 1){
        
            printf("Error al introducir los parámetros, vuelve a intentarlo.\n");
            scanf("%d %d", &network.N_PROCS, &network.N_SECS);
            
        }
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    network.mask = mask;

    act_int.sa_handler = handle_sigint;
    sigemptyset(&(act_int.sa_mask));
    act_int.sa_flags = 0;

    act_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    act_usr2.sa_handler = handle_sigusr2;
    sigemptyset(&(act_usr2.sa_mask));
    act_usr2.sa_flags = 0;

    act_term.sa_handler = handle_sigterm;
    sigemptyset(&(act_term.sa_mask));
    act_term.sa_flags = 0;

    act_alarm.sa_handler = handle_sigalarm;
    sigemptyset(&(act_alarm.sa_mask));
    act_alarm.sa_flags = 0;

    if (sigaction(SIGINT, &act_int, NULL)<0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }       

    if (sigaction(SIGTERM, &act_term, NULL)<0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }       

    if (sigaction(SIGALRM, &act_alarm, NULL)<0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }       

    if (sigaction(SIGUSR1, &act_usr1, NULL)<0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }       

    if (sigaction(SIGUSR2, &act_usr2, NULL)<0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }       

    // ALARM
    alarm(network.N_SECS);

    // INICIALIZAR LOS SEMÁFOROS
    if((sem1 = sem_open("/sem1", O_CREAT, 0644, 0))==SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_post(sem1);

    if((sem2 = sem_open("/sem2", O_CREAT, 0644, 0))==SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_post(sem2);

    // ABRIR FICHERO CON LOS PIDS
    f = fopen("PIDS", "w");
    if(f == NULL){
        perror("Abrir fichero");
        return EXIT_FAILURE;
    }

    /// CREAR PROCESOS VOTANTES
    for (i = 0; i< network.N_PROCS + 1; i++){
        pid= fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            votante(sem1, sem2, &network, f);
            exit(EXIT_SUCCESS);
        } else {
            // Proceso principal guarda el PID del votante
            network.pid[i] = pid;
        }
    }

    // ESCRIBIR EN EL FICHERO LOS PID
    for (i = 0; i<network.N_PROCS+1; i++){
        if(fprintf(f, "PID %d\n", network.pid[i]) == -1) {
            return EXIT_FAILURE;
        }
    }

    fclose(f);

    // ENVIAR SEÑALES A VOTANTES
    for (int i = 0; i < network.N_PROCS+1; i++) {
        if(kill(network.pid[i], SIGUSR1) == -1) {
            perror("kill");
            return EXIT_FAILURE;
        }
    }

    pause();
    sigwait(&mask, &sig);
    if (sig == SIGINT){
        
        for (int i = 0; i < network.N_PROCS+1; i++) {
            if(kill(network.pid[i], SIGTERM) == -1) {
                perror("kill");
                return EXIT_FAILURE;
            }
        }
        for (int i = 0; i < network.N_PROCS+1; i++) {
            if(waitpid(network.pid[i], NULL, 0) == -1) {
                perror("waitpid");
                return EXIT_FAILURE;
            }
        }
    }
    fflush(stdout);

    /*unlink("PIDS");*/
    fclose(f);
    sem_unlink("/sem1");
    sem_unlink("/sem2");
    sem_close(sem1);
    sem_close(sem2);
    return EXIT_SUCCESS;
}