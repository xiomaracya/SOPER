/**
 * @file monitor.c
 * @author Sara Serrano Marazuela
 * @author Xiomara Caballero Cuya
 * @brief Monitor
 * @version 1.0
 * @date 2025-04-15
 *
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "pow.h"
#include "monitor.h"
#include "minero.h"
#include <signal.h>
#include <sys/wait.h>

static volatile sig_atomic_t final = 0;

/**
 * @brief The handler of the signal SIGINT
 * 
 * @param sig the signal
 */
void handle_sigint(int sig) {
    (void)sig;
    final = 1;
}

/**
 * @brief The main that is going to execute monitor and comprobador
 *
 */
int main() {
    int fd_shm;
    bool fin = false;
    MemoriaCompartida *shm_block = NULL;

    struct sigaction act_int;
    act_int.sa_handler = handle_sigint;
    sigemptyset(&(act_int.sa_mask));
    act_int.sa_flags = 0;

    if(sigaction(SIGINT, &act_int, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while((fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR))==-1) {
        usleep(1000);
    }

    // Se establece el tamaño del segmento de memoria compartida
    if(ftruncate(fd_shm, sizeof(MemoriaCompartida)) == -1) {
        perror("ftruncate");
        close(fd_shm);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Hace un mapeo de la memoria compartida
    shm_block = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if(shm_block == MAP_FAILED) {
        perror("mmap");
        close(fd_shm);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Inicializa los semaforos de lectura de monitor
    sem_init(&shm_block->sem_empty, 1, MAX_BLOCKS);
    sem_init(&shm_block->sem_full, 1, 0);
    sem_init(&shm_block->sem_mutex, 1, 1);

    shm_block->in = 0;
    shm_block->out = 0;

    pid_t monitor_pid = fork();
    if (monitor_pid == 0) {
        /// MONITOR
        Block mensaje;

        while(!fin && !final) {
            // Se extrae un bloque
            sem_wait(&shm_block->sem_full);
            sem_wait(&shm_block->sem_mutex);

            mensaje = shm_block->buffer[shm_block->out];
            shm_block->out = (shm_block->out + 1) % MAX_BLOCKS;

            sem_post(&shm_block->sem_mutex);
            sem_post(&shm_block->sem_empty);

            fin = mensaje.fin;

            if (fin) break;

            // Se muestra el bloque por pantalla

            printf("Id:\t %04ld\n",mensaje.id );
            printf("Winner: %d\n", mensaje.ganador);
            printf("Target: %08ld\n", mensaje.objetivo);
            if (mensaje.flag) {
                printf("Solution: %08ld (validated)\n", mensaje.solucion);
            } else {
                printf("Solution: %08ld (rejected)\n", mensaje.solucion);
            }
            printf("Votes: %d/%d \n", mensaje.num_votos_positivos, mensaje.num_votos_totales);
            printf("Wallets: ");
            for (int i = 0; i<MAX_MINEROS; i++){
                if(mensaje.pid_carteras[i] != 0) {
                    printf(" %d:%d   ", mensaje.pid_carteras[i], mensaje.monedas[i]);
                }
            }
            printf("\n");
            fflush(stdout);

            // Se realiza la espera de lag milisegundos
            usleep(LAG*1000);
        }

        // Cuando recibe el bloque de finalización, libera los recursos y termina
        munmap(shm_block, sizeof(MemoriaCompartida));
        close(fd_shm);
        exit(EXIT_SUCCESS);

    } else if (monitor_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }else {
        /// COMPROBADOR

        mqd_t queue;
        Block mensaje;

        // Abrir la cola
        while((queue = mq_open(MQ_NAME, O_RDONLY)) == -1) {
            // Comprueba cada 100 ms
            usleep(LAG);
        }

        while (fin == false && final == false) {
            printf("Inicio de comprobador\n");
            fflush(stdout);
            // Recibe un bloque a través de la cola de mensajes
            if(mq_receive(queue, (char*)&mensaje, sizeof(Block), NULL) == -1) {
                perror("mq_receive");
                munmap(shm_block, sizeof(MemoriaCompartida));
                close(fd_shm);
                shm_unlink(SHM_NAME);
                mq_close(queue);
                mq_unlink(MQ_NAME);
                exit(EXIT_FAILURE);
            }

            // Comprueba si el bloque recibido es válido
            if(pow_hash(mensaje.solucion) == mensaje.objetivo) {
                mensaje.flag = true;
            } else {
                mensaje.flag = false;
            }
            fin = mensaje.fin;

            // Lo introducirá en memoria compartida para que lo lea Monitor
            sem_wait(&shm_block->sem_empty);
            sem_wait(&shm_block->sem_mutex);

            shm_block->buffer[shm_block->in] = mensaje;
            shm_block->in = (shm_block->in + 1) % MAX_BLOCKS;

            sem_post(&shm_block->sem_mutex);
            sem_post(&shm_block->sem_full);

            if(fin) break;
            
            usleep(LAG*1000);
        }

        //Comprobamos que se liberan los recursos si recibe SIGINT
        if (final && !fin) {
            mensaje.fin = true;
        
            sem_wait(&shm_block->sem_empty);
            sem_wait(&shm_block->sem_mutex);
        
            shm_block->buffer[shm_block->in] = mensaje;
            shm_block->in = (shm_block->in + 1) % MAX_BLOCKS;
        
            sem_post(&shm_block->sem_mutex);
            sem_post(&shm_block->sem_full);
        }

        int status;
        waitpid(monitor_pid, &status, 0);
        if (status != 0) {
            fprintf(stderr, "Monitor no terminó correctamente\n");
        }

        sem_destroy(&shm_block->sem_empty);
        sem_destroy(&shm_block->sem_full);
        sem_destroy(&shm_block->sem_mutex);


        munmap(shm_block, sizeof(MemoriaCompartida));
        close(fd_shm);

        mq_close(queue);
        mq_unlink(MQ_NAME);

        shm_unlink(SHM_NAME);
        printf("[%d] Finishing\n", getpid());
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }
       
}
