#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

/*Cosas que mejorar
*como utilizar en el handleer pids y N_PROCS
*tambien se podria devolver una flag para realizarlo desde el main
*/

void handle_sigint(int sig) {

    printf (" Signal number %d received \n", sig );
    exit(EXIT_SUCCESS);

}

void handle_sigusr1(int sig) {
    // handler para cuando un votante reciba SIGUSR1
    printf("Proceso hijo %d recibió %d\n", getpid(), sig);
    exit(EXIT_SUCCESS);
}

int main (int argc, char *argv[]){

    if(argc!=3){
        printf("Error al introducir los argumentos\n");
        return EXIT_FAILURE;
    }

    ///Inicializar
    FILE *f;
    int *pids;
    int i, nWriten, pid;
    int N_PROCS = atoi(argv[1]);
    int N_SECS = atoi(argv[2]);
    struct sigaction act_int, act_usr1;

    act_int.sa_handler = handle_sigint;
    sigemptyset(&(act_int.sa_mask));
    act_int.sa_flags = 0;

    act_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&(act_usr1.sa_mask));
    act_usr1.sa_flags = 0;

    pids = (int *)malloc(N_PROCS*sizeof(int));
    if(pids == NULL){
        return EXIT_FAILURE;
    }
 

    if (N_PROCS <= 0 || N_SECS <= 0) {
        perror("Error en los parámetros");
        return EXIT_FAILURE;
    }

    f = fopen("PIDS", "w");
    if(f == NULL){
        perror("Abrir fichero");
        return 1;
    }

    ///Crear procesos
    for (i = 0; i< N_PROCS; i++){
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

    for (i = 0; i<N_PROCS; i++){
        nWriten = fprintf(f, "Proceso %d con PID %d\n", i+1, pids[i]);
        if(nWriten == -1){
            return EXIT_FAILURE;
        }
    }

    ///enviar señal a votantes
    for (int i = 0; i < N_PROCS; i++) {
        kill(pids[i], SIGUSR1);
    }

    if (sigaction(SIGINT, &act_int, NULL)<0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }       

    printf("Esperando SIGINT\n");
    pause();

    for (int i = 0; i < N_PROCS; i++) {
        kill(pids[i], SIGTERM);
    }

    for (int i = 0; i < N_PROCS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("Finishing by signal\n");
    fflush(stdout);

    fclose(f);
    return EXIT_SUCCESS;
}