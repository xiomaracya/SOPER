#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "minero.h"
#include "monitor.h"

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
    int retorno_monitor;
    pid_t pid;
    pid_t pid2;

    int fd1[2];
    int fd2[2];
    int pipe_status;

    pipe_status = pipe(fd1);
    if (pipe_status == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pipe_status = pipe(fd2);
    if (pipe_status == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if(pid<0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        pid2 = fork();
        if(pid2<0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) {
            // MONITOR
            close(fd2[0]);
            close(fd1[1]);
            retorno_monitor = monitor(fd2[1], fd1[0]);
            close(fd2[1]);
            close(fd1[0]);
            exit(retorno_monitor);
        } else {
            // MINERO
            close(fd2[1]);
            close(fd1[0]);
            retorno_minero = proceso_minero(rondas, hilos, objetivo, fd1[1], fd2[0]);
            close(fd1[1]);
            close(fd2[0]);
            waitpid(pid2,&status,0);
            if (status == 1) {
                printf("Monitor exited unexpectedly\n");
            } else {
                printf("Monitor exited with status %d\n", status);
            }
            exit(retorno_minero);
        }
    } else {
        // PROCESO PRINCIPAL
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        waitpid(pid,&status,0);
        if (status == 1) {
            printf("Miner exited unexpectedly\n");
        } else {
            printf("Miner exited with status %d\n", status);
        }

    }

    exit(EXIT_SUCCESS);
}