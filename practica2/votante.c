#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#include <strings.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

volatile sig_atomic_t newVotante = 1;

int *readpids(int fd, int nProc) {
    int i;

    int bytes_leidos;
    char buffer[1024], *token;

    int *pids = (int*)malloc(MAX_PID*sizeof(int));
    if (pids == NULL) {
        return NULL;
    }

    fd = open(FICHERO, O_CREAT | O_RDONLY, 0644);
    if(fd == -1) {
        free(pids);
        return NULL;
    }
    bytes_leidos = read(fd, buffer, sizeof(buffer) - 1);
    if(bytes_leidos == -1) {
        perror("Error al leer el archivo");
        close(fd);
        free(pids);
        close(fd);
        return NULL;
    }
    buffer[bytes_leidos] = '\0'; 

    // Leer los pids
    token = strtok(buffer, "\n");
    printf("El primer token es %s\n", token);
    nProc = atoi(token);
    for(i= 0; i< nProc; i++){
        token = strtok(NULL, " ");
        printf("Otro token es %s", token);
        pids[i] = atoi(token);
    }

    close(fd);

    return pids;

}

char *readvotes(int fd, int nProc, int *ronda) {
    int i, j;

    int bytes_leidos;
    char buffer[1024], *token;

    int *pids = (int*)malloc(MAX_PID*sizeof(int));
    if (pids == NULL) {
        return NULL;
    }

    char *votes = (char*)malloc(MAX_PID*sizeof(char));
    if(votes == NULL) {
        free(pids);
        return NULL;
    }

    fd = open(FICHERO, O_CREAT | O_RDONLY, 0644);
    if(fd == -1) {
        free(pids);
        free(votes);
        return NULL;
    }

    *ronda+=1;
    printf("Esta es la ronda %d\n", *ronda);
    bytes_leidos = read(fd, buffer, sizeof(buffer) - 1);
    if(bytes_leidos == -1) {
        perror("Error al leer el archivo");
        close(fd);
        free(pids);
        free(votes);
        close(fd);
        return NULL;
    }
    buffer[bytes_leidos] = '\0'; 

    // Leer los pids
    token = strtok(buffer, "\n");
    printf("El primer token es %s\n", token);
    nProc = atoi(token);
    for(i= 0; i< nProc; i++){
        token = strtok(NULL, " ");
        printf("Otro token es %s", token);
        pids[i] = atoi(token);
    }
    token = strtok(NULL, "\n");
    printf("Otro token es %s", token);
    pids[i] = atoi(token);
    for(j=0; j < *ronda; j++){
        printf("Escribimos la ronda %d\n", *ronda);
        for (i = 0; i < nProc-1 && token != NULL; i++) {
            printf("OK\n");
            printf("El buffer es %s", buffer);
            token = strtok(NULL, " ");  // Coge el pid
            printf("El segundo token es %s\n", token);
            token = strtok(NULL, " ");  // Coge la palabra "vota"
            printf("El tercer token es %s\n", token);
            token = strtok(NULL, "\n");  // Dividir por espacios
            votes[i] = token[0];  // Almacenar el voto (Y o N)
            printf("Se ha guardado %c\n", votes[i]);
        }
    }

    return votes;

}

int chooseCandidato(int fd, sem_t *sem1, sem_t *sem2, sem_t *sem3, int nProc, sigset_t mask, int *ronda){
    char answer;
    int i;
    int yes = 0, no = 0;
    int sig, val, *pids;
    char *votes;

    // LEEMOS LOS PIDS DEL FICHERO
    pids = readpids(fd, nProc);
    if(pids == NULL) {
        return 1;
    }
    // SEMÁFORO = 1 -> CANDIDATO
    if(sem_trywait(sem1) == 0) {
        printf("Candidato\n");

        sem_getvalue(sem3, &val);
        while (val>0) {
            sem_getvalue(sem3, &val);
        }

        for (int i = 0; i < nProc+1; i++) {
            newVotante = 0;
            if(pids[i] != getpid()) {
                fflush(stdout);
                kill(pids[i], SIGUSR2);
                printf("Señal SIGUSR2 enviada\n");
            }
        }

        fd = open(FICHERO, O_CREAT | O_RDONLY, 0644);

        votes = readvotes(fd, nProc, ronda);
        usleep(1000);
        printf("OKKKK<ºn");

        printf("Candidate %d => [", getpid());
        for (int i = 0; i < nProc; i++) {
            printf(" %c ", votes[i]);
        }
        printf("] => ");
        
        for (i=0; i<nProc; i++) {
            if (votes[i] == 'Y') {
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
        usleep(250000);


        printf("New votante a 1");
        newVotante = 1;

        sem_getvalue(sem2, &val);
        while (val != 0) {
            sem_wait(sem2);
            sem_getvalue(sem2, &val);
        }
        sem_post(sem2);
        printf("OK2\n");

        sem_getvalue(sem3, &val);
        while (val != nProc) {
            sem_post(sem3);
            sem_getvalue(sem3, &val);
        }
        printf("OK3\n");

        // Enviar SIGUSR1 a cada proceso votante no candidato
        for (int i = 0; i < nProc + 1; i++) {
            if (kill(pids[i], SIGUSR1) == -1) {
                perror("Error al enviar SIGUSR1");
            }
        }

        sem_post(sem1);
        return 0;
    /// SEMAFORO = 0 -> VOTANTES
    } else {
        printf("Votante\n");

        sem_wait(sem3);
        sigaddset(&mask, SIGUSR2);
        if (sigwait(&mask, &sig) == 0) {
            if (sig == SIGUSR2) {
                printf("Recibí SIGUSR2\n");
            }
        } else {
            perror("sigwait"); 
            exit(EXIT_FAILURE);
        }
        sem_wait(sem2);

        srand(getpid());
        if(rand() % 2==0) {
            answer = 'N';
        } else {
            answer = 'Y';
        }

        fd = open(FICHERO, O_WRONLY | O_APPEND , 0644);  // Abrir archivo para añadir votos en modo sobreescritura
        if (fd == -1) {
            perror("Error al abrir el archivo");
            return EXIT_FAILURE;
        }

        dprintf(fd, "%d vota %c\n", getpid(), answer);
        fsync(fd);
        close(fd);

        sem_post(sem2);
        printf("Proceso votante terminado\n");

        sigdelset(&mask, SIGUSR2);
        if (sigwait(&mask, &sig) == 0) {
            if (sig == SIGUSR1) {
                printf("Recibí SIGUSR1\n");
            }
        } else {
            perror("sigwait"); 
            exit(EXIT_FAILURE);
        }

        return 0;
    }

}

int votante(int fd, sem_t *sem1, sem_t *sem2, sem_t *sem3, int nProc, sigset_t mask) {
    struct sigaction act_int, act_usr1, act_usr2, act_term, act_alarm;
    int sig;
    sigset_t old_mask;
    int ronda = 0;

    // Inicializar acciones para las señales
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

    // Configurar sigactions
    if (sigaction(SIGINT, &act_int, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &act_term, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGALRM, &act_alarm, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR1, &act_usr1, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &act_usr2, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Bloquear señales durante las oper aciones críticas
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);

    // Esperar señales
    while (1) {
        sigwait(&mask, &sig);

        if (sig == SIGUSR1) {
            printf("PID %d: Recibí SIGUSR1\n", getpid());
            // Repetir proceso de votación si hay un nuevo votante
            while (newVotante) {
                printf("Vuelve a empezar el bucle\n");
                sleep(1);
                chooseCandidato(fd, sem1, sem2, sem3, nProc, mask, &ronda);
                printf("Un proceso ha terminado choose\n");
                printf("continua el bucle\n");
            }
            printf("Se ha salido del bucle\n");
        }

        // Uso de sigsuspend para esperar señales sin consumir CPU
        sigsuspend(&old_mask);  // Esperar señal mientras bloqueamos las señales en old_mask
    }

    exit(EXIT_SUCCESS);
}
