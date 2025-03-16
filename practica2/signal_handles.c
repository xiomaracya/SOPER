#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

void handle_sigint(int sig) {

    printf (" Signal number %d received \n", sig );
    exit(EXIT_SUCCESS);

}

void handle_sigterm(int sig) {

    printf (" Signal number %d received \n", sig );
    exit(EXIT_SUCCESS);

}

void handle_sigalarm(int sig) {

    printf (" Signal number %d received \n", sig );
    exit(EXIT_SUCCESS);

}

void handle_sigusr1(int sig) {
    // handler para cuando un votante reciba SIGUSR1
    printf("Proceso hijo %d recibió %d\n", getpid(), sig);
    exit(EXIT_SUCCESS);
}

void handle_sigusr2(int sig) {
    // handler para cuando un votante reciba SIGUSR1
    printf("Proceso hijo %d recibió %d\n", getpid(), sig);
    exit(EXIT_SUCCESS);
}