#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#define MAX_PID 30

/*Cosas que mejorar
*como utilizar en el handleer pids y N_PROCS
*tambien se podria devolver una flag para realizarlo desde el main
*/

typedef struct () {
    bool yes_or_no;
} Votes;

typedef struct () {
    pid_t pid[MAX_PID];
    Votes vote[MAX_PID];
    int N_PROCS;
    int N_SECS;
    _sigset_t mask;

} Network;

int main (int argc, char *argv[]){

    network.N_PROC = atoi(argv[1]);
    network.N_SECS = atoi(argv[2]);

    while(nArg != 3 || network.N_PROCS < 1 || network.N_SECs < 1){
        
        printf("Error al introducir los parámetros, vuelve a intentarlo.\n");
        scanf("%d %d", network.NPROCS, network.N_SECS);
        
    }

    ///Inicializar
    FILE *f;
    int *pids;
    int i, nWriten, pid;
    struct sigaction act_int, act_usr1, act_usr2, act_term, act_alarm;
    Network network;

    act_int.sa_handler = handle_sigint;
    sigemptyset(&(act_int.sa_mask));
    act_int.sa_flags = 0;

    act_term.sa_handler = handle_sigterm;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    act_alarm.sa_handler = handle_sigalarm;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    act_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    act_usr2.sa_handler = handle_sigusr2;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    pids = (int *)malloc(network.N_PROC*sizeof(int));
    if(pids == NULL){
        return EXIT_FAILURE;
    }
 

    if (network.N_PROC <= 0 || network.N_SEC <= 0) {
        perror("Error en los parámetros");
        return EXIT_FAILURE;
    }

    f = fopen("PIDS", "w");
    if(f == NULL){
        perror("Abrir fichero");
        return 1;
    }

    ///Crear procesos
    for (i = 0; i< network.N_PROC; i++){
        pid= fork();
        if (pid == 0) {
            if (sigaction(SIGUSR1, &act_usr1, NULL) < 0) {
                perror("sigaction SIGUSR1");
                return EXIT_FAILURE;
            }
            pause();  // Espera recibir SIGUSR1
            exit(EXIT_SUCCESS);
        } else {
            // Proceso principal guarda el PID del votante
            pids[i] = pid;
    
        }
    }

    for (i = 0; i<network.N_PROC; i++){
        nWriten = fprintf(f, "Proceso %d con PID %d\n", i+1, pids[i]);
        if(nWriten == -1){
            return EXIT_FAILURE;
        }
    }

    ///enviar señal a votantes
    for (int i = 0; i < network.N_PROC; i++) {
        kill(pids[i], SIGUSR1);
    }

    if (sigaction(SIGINT, &act_int, NULL)<0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }       

    printf("Esperando SIGINT\n");
    pause();

    for (int i = 0; i < network.N_PROC; i++) {
        kill(pids[i], SIGTERM);
    }

    for (int i = 0; i < network.N_PROC; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("Finishing by signal\n");
    fflush(stdout);

    fclose(f);
    return EXIT_SUCCESS;
}