#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "minero.h"

int main(int argc, char *argv[]){

    if(argc!=4){
        printf("Debes introducir tres argumentos\n");
        exit(EXIT_FAILURE);
    }

    int rondas = atoi(argv[2]);
    int hilos = atoi(argv[3]);
    long int objetivo = atol(argv[1]);
    int status;
    int retorno_minero;
    pid_t pid;

    pid = fork();
    if(pid<0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        pid = fork();
        if(pid<0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // MONITOR
            printf("Monitor\n");
            exit(EXIT_SUCCESS);
        } else {
            // MINERO
            retorno_minero = proceso_minero(rondas, hilos, objetivo);
            wait(&status);
            if (status == 1) {
                printf("Monitor exited unexpectedly\n");
            } else {
                printf("Monitor exited with status %d\n", status);
            }
            exit(retorno_minero);
        }
    } else {
        // PROCESO PRINCIPAL
        wait(&status);
        if (status == 1) {
            printf("Miner exited unexpectedly\n");
        } else {
            printf("Miner exited with status %d\n", status);
        }

    }

    exit(EXIT_SUCCESS);
}