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

int chooseCandidato(sem_t *sem1, sem_t *sem2, sem_t *sem3, sem_t *sem4, Network *network){
    char buffer[1024];
    char answer, *token;
    int i, flag=0, fd, bytes_leidos;
    int yes = 0, no = 0;
    int sig, val, val2, nProc, pids[MAX_PID];
    sigset_t final_mask;

    
    // SEMÁFORO = 1 -> CANDIDATO
    if(sem_trywait(sem1) == 0) {
        printf("Candidato\n");

        sem_getvalue(sem3, &val);
        while (val>0) {
            sem_getvalue(sem3, &val);
        }

        for (int i = 0; i < network->N_PROCS+1; i++) {
            newVotante = 0;
            if(network->pid[i] != getpid()) {
                fflush(stdout);
                kill(network->pid[i], SIGUSR2);
                printf("Señal SIGUSR2 enviada\n");
            }
        }

        fd = open("PIDS.txt", O_CREAT | O_RDONLY, 0644);
        while (flag==0) {
            flag = 1;
            bytes_leidos = read(fd, buffer, sizeof(buffer) - 1);
            if(bytes_leidos == -1) {
                perror("Error al leer el archivo");
                close(fd);
                return EXIT_FAILURE;
            }
            buffer[bytes_leidos] = '\0'; 

            // Convertir los votos de la tercera línea
            token = strtok(buffer, "\n");
            nProc = atoi(token);
            for(i= 0; i< nProc +1; i++){
                token = strtok(NULL, " ");
                pids[i] = atoi(token);
            }
            for (i = 0; i < network->N_PROCS && token != NULL; i++) {
                token = strtok(NULL, " ");  // Coge el pid
                token = strtok(NULL, " ");  // Coge la palabra "vota"
                token = strtok(NULL, "\n");  // Dividir por espacios
                network->vote[i] = token[0];  // Almacenar el voto (Y o N)
            }
            usleep(1000);
        }
        close(fd);

        printf("Candidate %d => [", getpid());
        for (int i = 0; i < network->N_PROCS; i++) {
            printf(" %c ", network->vote[i]);
        }
        printf("] => ");
        
        for (i=0; i<network->N_PROCS; i++) {
            if (network->vote[i] == 'Y') {
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

        // Enviar SIGUSR1 a cada proceso votante no candidato
        for (int i = 0; i < network->N_PROCS + 1; i++) {
            if (kill(pids[i], SIGUSR1) == -1) {
                perror("Error al enviar SIGUSR1");
            }
        }

        sem_post(sem1);

        printf("New votante a 1");
        newVotante = 1;
        for (i=0; i<network->N_PROCS+1; i++) {
            sem_post(sem4);
        }

        sem_getvalue(sem1, &val);
        while (val != 0) {
            sem_wait(sem1);
            sem_getvalue(sem1, &val);
        }
        sem_post(sem1);
        printf("OK1\n");

        sem_getvalue(sem2, &val);
        while (val != 0) {
            sem_wait(sem2);
            sem_getvalue(sem2, &val);
        }
        sem_post(sem2);
        printf("OK2\n");

        sem_getvalue(sem3, &val);
        while (val != network->N_PROCS) {
            sem_post(sem3);
            sem_getvalue(sem3, &val);
        }
        printf("OK3\n");
        return 0;
    /// SEMAFORO = 0 -> VOTANTES
    } else {
        printf("Votante\n");

        sem_wait(sem3);
        if (sigwait(&network->mask, &sig) == 0) {
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

        fd = open("PIDS.txt", O_WRONLY | O_APPEND , 0644);  // Abrir archivo para añadir votos en modo sobreescritura
        if (fd == -1) {
            perror("Error al abrir el archivo");
            return EXIT_FAILURE;
        }

        dprintf(fd, "%d vota %c\n", getpid(), answer);
        fsync(fd);
        close(fd);

        sem_post(sem2);
        printf("Proceso votante terminado\n");

        return 0;
    }

}

int votante(sem_t *sem1, sem_t *sem2, sem_t *sem3, sem_t *sem4, Network *network) {
    struct sigaction act_int, act_usr1, act_usr2, act_term, act_alarm;
    int sig;
    sigset_t new_mask, old_mask;
    int current_value, current_value2;

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

    // Bloquear señales durante las operaciones críticas
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    sigaddset(&new_mask, SIGUSR2);
    sigaddset(&new_mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

    // Esperar señales
    while (1) {
        sigwait(&network->mask, &sig);

        if (sig == SIGUSR1) {
            printf("PID %d: Recibí SIGUSR1\n", getpid());
            // Repetir proceso de votación si hay un nuevo votante
            while (newVotante) {
                printf("Vuelve a empezar el bucle\n");
                sem_getvalue(sem4, &current_value);
                while (current_value > 0) {
                    sem_getvalue(sem4, &current_value);
                    usleep(100);
                }
                printf("El semáforo 4 se ha puesto a 0");
                sleep(1);
                chooseCandidato(sem1, sem2, sem3, sem4, network);
                printf("Un proceso ha terminado choose\n");
                sem_wait(sem4);
                sem_getvalue(sem4, &current_value2);
                printf("current value: %d\n", current_value2);
                printf("continua el bucle\n");
            }
            printf("Se ha salido del bucle\n");
        }

        // Uso de sigsuspend para esperar señales sin consumir CPU
        sigsuspend(&old_mask);  // Esperar señal mientras bloqueamos las señales en old_mask
    }

    exit(EXIT_SUCCESS);
}
