/**
 * @brief Creará los procesos votante y controlará si el sistema está listo
 * 
 * @file principal.calloc
 * @author Xiomara Caballero Cuya, Sara Serrano Marazuela
 * @version 1.0
 * @date 17/03/2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "votante.h"

volatile sig_atomic_t terminar = 0;


void handle_sigint(int sig) {
    (void)sig;
    int fd, num_procesos=0, *pids=NULL;
    int i;
    
    fd = open("PIDS.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }


    pids = readpids(fd, &num_procesos);
    if(pids == NULL) {
        perror("readpids");
        exit(EXIT_FAILURE);
    }

    if(getpid() == pids[0]) {
        printf("Finishing by signal\n");
    }
    fflush(stdout);
    usleep(250000);

    close(fd);  // Cerrar el fichero

    for (i = 0; i < num_procesos+1; i++) {
        if (kill(pids[i], SIGTERM) == -1) {
            perror("kill");
            free(pids);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < num_procesos+1; i++) {
        wait(NULL);
    }

    free(pids);

    unlink(FICHERO);
    sem_unlink("/sem1");
    sem_unlink("/sem2");
    sem_unlink("/sem3");

    //unlink(FICHERO);
    exit(EXIT_SUCCESS);
}


void handle_sigterm(int sig) {
    (void)sig;
    terminar = 1;
    
    /*unlink("PIDS");*/
    unlink("/sem1");
    unlink("/sem2");
    unlink("/sem3");
    exit(EXIT_SUCCESS);

}


void handle_sigalarm(int sig) {
    (void)sig;
    int fd, num_procesos=0, *pids=NULL;
    int i;
    printf("Finishing by alarm\n");
    
    fd = open("PIDS.txt", O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir el archivo de PIDs");
        exit(EXIT_FAILURE);
    }

    pids = readpids(fd, &num_procesos);
    if(pids == NULL) {
        perror("readpids");
        exit(EXIT_FAILURE);
    }

    close(fd);  // Cerrar el fichero

    // Enviar SIGTERM a cada proceso votante
    for (i = 0; i < num_procesos; i++) {
        if (kill(pids[i], SIGTERM) == -1) {
            perror("kill");
            free(pids);
            exit(EXIT_FAILURE);
        }
    }


    sem_unlink("/sem1");
    sem_unlink("/sem2");
    sem_unlink("/sem3");
    unlink(FICHERO);
    exit(EXIT_SUCCESS);
    
}


void handle_sigusr1(int sig) {
    (void)sig;
}

void handle_sigusr2(int sig) {
    (void)sig;
}


/**
 * @brief Función main
 * 
 * Crea los procesos votante
 * Almacena los pids
 * Envía SIGUSR1 cuando está lsito
 * Envía SIGTERM cuando recibe SIGINT o SIGALARM
 */
int main (int argc, char *argv[]){

    sem_t *sem1 = NULL;
    sem_t *sem2 = NULL;
    sem_t *sem3 = NULL;
    int val2, nProc=0, nSec=0;

    // INICIALIZAR HANDLES
    int i, fd;
    pid_t pid;
    struct sigaction act_int, act_usr1, act_usr2, act_term, act_alarm;
    sigset_t mask;
    int pids[MAX_PID];

    if(argc==3) {
        nProc = atoi(argv[1]);
        nSec = atoi(argv[2]);
    }

    // CONTROL DE PARÁMETROS
    if (argc != 3 || nProc < 1 || nSec < 1 || nProc >= MAX_PID) {
        while(nProc < 1 || nSec < 1|| nProc >=  MAX_PID){
        
            printf("Error al introducir los parámetros, vuelve a intentarlo.\n");
            if(nProc < 1 || nProc >= MAX_PID){
                printf("El número de votantes debe encontrar entre 1 y 29\n");
            }
            if(nSec < 1){
                printf("Los segundos deben ser mayor que 0\n");
            }
            
            scanf("%d %d", &nProc, &nSec);
            
        }
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

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
    alarm(nSec);

    sem_unlink("/sem1");
    sem_unlink("/sem2");
    sem_unlink("/sem3");

    // INICIALIZAR LOS SEMÁFOROS
    if((sem1 = sem_open("/sem1", O_CREAT, S_IRUSR | S_IWUSR, 0))==SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_post(sem1);

    if((sem2 = sem_open("/sem2", O_CREAT, S_IRUSR | S_IWUSR, 0))==SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_post(sem2);

    if((sem3 = sem_open("/sem3", O_CREAT, S_IRUSR | S_IWUSR, nProc))==SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_getvalue(sem3, &val2);

    // ABRIR FICHERO CON LOS PIDS
    fd = open(FICHERO, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    dprintf(fd, "%d\n", nProc);

    /// CREAR PROCESOS VOTANTES
    for (i = 0; i< nProc + 1; i++){
        pid= fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            votante(fd, sem1, sem2, sem3, nProc, mask);
            exit(EXIT_SUCCESS);
        } else {
            // Proceso principal guarda el PID del votante
            pids[i] = pid;
        }
    }

    // ESCRIBIR EN EL FICHERO LOS PID
    for (i = 0; i<nProc+1; i++){
        if(dprintf(fd, "%d ", pids[i]) == -1) {
            return EXIT_FAILURE;
        }
    }

    dprintf(fd, "\n");
    close(fd);

    // ENVIAR SEÑALES A VOTANTES
    for (i = 0; i < nProc+1; i++) {
        if(kill(pids[i], SIGUSR1) == -1) {
            perror("kill");
            return EXIT_FAILURE;
        }
    }

    while (!terminar);
}