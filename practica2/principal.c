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

void handle_sigint(int sig) {

    int fd, num_procesos, pids[MAX_PID];

    printf("Finishing by signal\n");
    
    fd = open("PIDS", O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir el archivo de PIDs");
        exit(EXIT_FAILURE);
    }

    // Leer los PIDs del fichero
    char buffer[256];  // Buffer temporal para leer el archivo
    long int bytes_leidos = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_leidos <= 0) {
        perror("Error al leer el archivo de PIDs");
        close(fd);
        exit(EXIT_FAILURE);
    }

    buffer[bytes_leidos] = '\0';  // Asegurar terminación de cadena
    char *token = strtok(buffer, "\n");
    num_procesos = atoi(token);

    close(fd);  // Cerrar el fichero

    // Convertir el contenido del archivo en PIDs
    token = strtok(buffer, "\n");  
    while (token != NULL && num_procesos < MAX_PID) {
        pids[num_procesos++] = atoi(token);  // Convertir a entero
        token = strtok(NULL, "\n");
    }

    // Enviar SIGTERM a cada proceso votante
    for (int i = 0; i < num_procesos; i++) {
        if (kill(pids[i], SIGTERM) == -1) {
            perror("Error al enviar SIGTERM");
        }
    }
}

void handle_sigterm(int sig) {
    
    /*unlink("PIDS");*/
    unlink("/sem1");
    unlink("/sem2");
    printf("Freeing...");
    exit(EXIT_SUCCESS);

}

void handle_sigalarm(int sig) {
    printf("Finishing by alarm\n");
    exit(EXIT_SUCCESS);

}

void handle_sigusr1(int sig) {
    printf("Recibido SIGUSR1 en el manejador\n");
}

void handle_sigusr2(int sig) {


}

int main (int argc, char *argv[]){

    sem_t *sem1 = NULL;
    sem_t *sem2 = NULL;
    sem_t *sem3 = NULL;
    int val2;

    // INICIALIZAR HANDLES
    int i, fd;
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

    sem_unlink("/sem1");
    sem_unlink("/sem2");
    sem_unlink("/sem3");

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

    if((sem3 = sem_open("/sem3", O_CREAT, 0644, network.N_PROCS))==SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_getvalue(sem3, &val2);
        printf("INICIAL%d\n",val2);

    // ABRIR FICHERO CON LOS PIDS
    fd = open("PIDS.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    dprintf(fd, "%d\n", network.N_PROCS);

    /// CREAR PROCESOS VOTANTES
    for (i = 0; i< network.N_PROCS + 1; i++){
        pid= fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            votante(sem1, sem2, sem3, &network);
            exit(EXIT_SUCCESS);
        } else {
            // Proceso principal guarda el PID del votante
            network.pid[i] = pid;
        }
    }

    // ESCRIBIR EN EL FICHERO LOS PID
    for (i = 0; i<network.N_PROCS+1; i++){
        if(dprintf(fd, "%d ", network.pid[i]) == -1) {
            return EXIT_FAILURE;
        }
    }

    dprintf(fd, "\n");
    close(fd);

    // ENVIAR SEÑALES A VOTANTES
    for (int i = 0; i < network.N_PROCS+1; i++) {
        if(kill(network.pid[i], SIGUSR1) == -1) {
            perror("kill");
            return EXIT_FAILURE;
        }
    }

    while(1);

    /*unlink("PIDS");*/
    close(fd);
    sem_unlink("/sem1");
    sem_unlink("/sem2");
    sem_unlink("/sem3");
    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);
    return EXIT_SUCCESS;
}