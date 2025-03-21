/**
 * @brief Ejecutará el procedimiento de los procesos votante, tanto en caso de que un proceso se elija como candidato, como si tiene que escribir su voto en el fichero
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
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#include <strings.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

volatile sig_atomic_t newVotante = 1;


int *readpids(int fd, int *nProc) {
    int i;

    int bytes_leidos;
    char buffer[1024], *token;

    int *pids = (int*)malloc(MAX_PID*sizeof(int));
    if (pids == NULL) {
        perror("malloc");
        return NULL;
    }

    fd = open(FICHERO, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        perror("open");
        free(pids);
        return NULL;
    }
    bytes_leidos = read(fd, buffer, sizeof(buffer) - 1);
    if(bytes_leidos == -1) {
        perror("read");
        free(pids);
        close(fd);
        return NULL;
    }
    buffer[bytes_leidos] = '\0'; 

    // Leer los pids
    token = strtok(buffer, "\n");
    *nProc = atoi(token);
    for(i= 0; i< *nProc +1; i++){
        token = strtok(NULL, " ");
        if(token == NULL) {
            perror("strtok");
            free(pids);
            close(fd);
            return NULL;
        }
        pids[i] = atoi(token);
        if(pids[i]<0) {
            perror("atoi");
            free(pids);
            close(fd);
            return NULL;
        }
    }

    close(fd);

    return pids;

}


char *readvotes(int fd, int *nProc, int *ronda) {
    int i, j, flag;

    int bytes_leidos;
    char buffer[1024], *token;

    int *pids = (int*)malloc(MAX_PID*sizeof(int));
    if (pids == NULL) {
        perror("malloc");
        return NULL;
    }

    char *votes = (char*)malloc(MAX_PID*sizeof(char));
    if(votes == NULL) {
        perror("malloc");
        free(pids);
        return NULL;
    }

    fd = open(FICHERO, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        perror("open");
        free(pids);
        free(votes);
        return NULL;
    }

    /*ronda = *ronda + 1;*/
    bytes_leidos = read(fd, buffer, sizeof(buffer) - 1);
    if(bytes_leidos == -1) {
        perror("read");
        close(fd);
        free(pids);
        free(votes);
        return NULL;
    }
    buffer[bytes_leidos] = '\0'; 

    // Leer los pids
    token = strtok(buffer, "\n");
    *nProc = atoi(token);
    for(i= 0; i< *nProc; i++){
        token = strtok(NULL, " ");
        pids[i] = atoi(token);
    }
    token = strtok(NULL, "\n");
    pids[i] = atoi(token);

    flag=1;
    while (flag==1) {
        flag=0;
        for(j=0; j < *ronda-1; j++){
            for (i = 0; i < *nProc && token != NULL; i++) {
                token = strtok(NULL, " ");  // Coge el pid
                token = strtok(NULL, " ");  // Coge la palabra "vota"
                token = strtok(NULL, "\n");  // Dividir por espacios
                votes[i] = token[0];  // Almacenar el voto (Y o N)
            }
        }
        for (i = 0; i < *nProc && token != NULL; i++) {
            token = strtok(NULL, " ");  // Coge el pid
            if(token == NULL) {
                flag = 1;
            } else {
                token = strtok(NULL, " ");  // Coge la palabra "vota"
                if (token == NULL) {
                    flag = 1;
                } else {
                    token = strtok(NULL, "\n");  // Dividir por espacios
                    if (token == NULL) {
                        flag=1;
                    } else {
                        votes[i] = token[0];  // Almacenar el voto (Y o N)
                    }
                }
            }
        }
    }

    free(pids);
    close(fd);
    return votes;

}


int chooseCandidato(int fd, sem_t *sem1, sem_t *sem2, sem_t *sem3, int nProc, sigset_t mask, int *ronda){
    char answer;
    int i;
    int yes = 0, no = 0;
    int sig, val, *pids = NULL;
    char *votes = NULL;

    // LEEMOS LOS PIDS DEL FICHERO
    pids = readpids(fd, &nProc);
    if(pids == NULL) {
        perror("readpids");
        return 1;
    }
    // SEMÁFORO = 1 -> CANDIDATO
    if(sem_trywait(sem1) == 0) {

        if(sem_getvalue(sem3, &val) == -1) {
            free(pids);
            return 1;
        }
        while (val>0) {
            if(sem_getvalue(sem3, &val) == -1) {
                free(pids);
                return 1;
            }
        }

        for (int i = 0; i < nProc+1; i++) {
            newVotante = 0;
            if(pids[i] != getpid()) {
                if(kill(pids[i], SIGUSR2) == -1) {
                    perror("kill");
                    free(pids);
                    return 1;
                }
            }
        }

        fd = open(FICHERO, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
        if(fd == -1) {
            perror("open");
            free(pids);
            return 1;
        }

        int escrito = 0;

        while(escrito==0){
            escrito = 1;
            votes = readvotes(fd, &nProc, ronda);
            if(votes == NULL) {
                perror("readvotes");
                free(pids);
                close(fd);
                return 1;
            }
            for (i =0; i<nProc; i++){
                if (votes[i] != 'Y' && votes[i] != 'N'){
                    escrito = 0;
                    break;
                }
            }
            usleep(1000);
        }

        close(fd);

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

        newVotante = 1;

        if(sem_getvalue(sem2, &val) == -1) {
            free(pids);
            free(votes);
            close(fd);
            return 1;
        }
        while (val != 0) {
            sem_wait(sem2);
            if(sem_getvalue(sem2, &val) == -1) {
                free(pids);
                free(votes);
                close(fd);
                return 1;
            }
        }
        sem_post(sem2);

        if(sem_getvalue(sem3, &val) == -1) {
            free(pids);
            free(votes);
            close(fd);
            return 1;
        }
        while (val != nProc) {
            sem_post(sem3);
            if(sem_getvalue(sem3, &val) == -1) {
                free(pids);
                free(votes);
                close(fd);
                return 1;
            }
        }

        // Enviar SIGUSR1 a cada proceso votante no candidato
        for (int i = 0; i < nProc + 1; i++) {
            if (kill(pids[i], SIGUSR1) == -1) {
                free(pids);
                free(votes);
                close(fd);
                return 1;
            }
        }

        free(pids);
        free(votes);
        sem_post(sem1);
        return 0;
    /// SEMAFORO = 0 -> VOTANTES
    } else {

        sem_wait(sem3);
        sigaddset(&mask, SIGUSR2);
        if (sigwait(&mask, &sig) != 0) {
            perror("sigwait"); 
            free(pids);
            close(fd);
            return 1;
        }
        sem_wait(sem2);

        srand(getpid()+time(NULL));
        if(rand() % 2==0) {
            answer = 'N';
        } else {
            answer = 'Y';
        }

        fd = open(FICHERO, O_WRONLY | O_APPEND , S_IRUSR | S_IWUSR);  // Abrir archivo para añadir votos en modo sobreescritura
        if (fd == -1) {
            perror("open");
            free(pids);
            close(fd);
            return EXIT_FAILURE;
        }

        dprintf(fd, "%d vota %c\n", getpid(), answer);
        close(fd);

        sem_post(sem2);

        sigdelset(&mask, SIGUSR2);
        if (sigwait(&mask, &sig) != 0) {
            perror("sigwait"); 
            free(pids);
            close(fd);
            exit(EXIT_FAILURE);
        }

        free(pids);
        free(votes);
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
            // Repetir proceso de votación si hay un nuevo votante
            while (newVotante) {
                sleep(1);
                ronda ++;
                if(chooseCandidato(fd, sem1, sem2, sem3, nProc, mask, &ronda) == 1) {
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Uso de sigsuspend para esperar señales sin consumir CPU
        sigsuspend(&old_mask);  // Esperar señal mientras bloqueamos las señales en old_mask
    }

    exit(EXIT_SUCCESS);
}
