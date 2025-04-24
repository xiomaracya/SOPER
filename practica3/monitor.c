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

/**
 * @brief The main that is going to execute monitor and comprobador
 *
 * @param argc número de argumentos
 * @param argv argumentos
 */
int main(int argc, char* argv[]) {
    int fd_shm;
    int lag;
    bool fin = false;
    MemoriaCompartida *shm_block = NULL;

    if(argc == 2) {
        lag = atoi(argv[1]);
    } else {
        printf("Error en los argumentos del ejecutable monitor\n");
        return EXIT_FAILURE;
    }
    if(lag < 0){
        printf("Error en los argumentos del ejecutable monitor\n");
        return EXIT_FAILURE;
    }

    fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if(fd_shm == -1) {
        Block mensaje;
        // MONITOR

        // Se abre un segmento de memoria compartida
        fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
        if(fd_shm == -1) {
            perror("Error al abrir el segmento de memoria compartida\n");
            return EXIT_FAILURE;
        }

        shm_block = mmap(NULL, sizeof(MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
        if(shm_block == MAP_FAILED) {
            perror("mmap");
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        printf("[%d] Printing blocks...\n", getpid());
        fflush(stdout);

        while(fin == false) {
            // Se extrae un bloque
            sem_wait(&shm_block->sem_full);
            sem_wait(&shm_block->sem_mutex);

            mensaje = shm_block->buffer[shm_block->out];
            shm_block->out = (shm_block->out + 1) % MAX_BLOCKS;

            sem_post(&shm_block->sem_mutex);
            sem_post(&shm_block->sem_empty);

            fin = mensaje.fin;

            // Se muestra el bloque por pantalla

            if(mensaje.flag==true) {
                printf("Solution accepted: %08ld --> %08ld\n", mensaje.objetivo, mensaje.solucion);
                fflush(stdout);
            } else {
                printf("Solution rejected: %08ld !-> %08ld\n", mensaje.objetivo, mensaje.solucion);
                fflush(stdout);
            }

            // Se realiza la espera de lag milisegundos
            usleep(lag*1000);
        }

        // Cuando recibe el bloque de finalización, libera los recursos y termina
        munmap(shm_block, sizeof(MemoriaCompartida));
        close(fd_shm);
        shm_unlink(SHM_NAME);

    } else {
        mqd_t queue;
        Block mensaje;
        
        // COMPROBADOR
        
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

        // Inicializa los semáforos
        sem_init(&shm_block->sem_empty, 1, MAX_BLOCKS);
        sem_init(&shm_block->sem_full, 1, 0);
        sem_init(&shm_block->sem_mutex, 1, 1);
        shm_block->in = 0;
        shm_block->out = 0;

        close(fd_shm);

        // Abrir la cola
        while((queue = mq_open(MQ_NAME, O_RDONLY)) == -1) {
            // Comprueba cada 100 ms
            usleep(100);
        }

        printf("[%d] Checking blocks...\n", getpid());
        fflush(stdout);

        while (fin == false) {
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
            
            usleep(lag*1000);
        }

        munmap(shm_block, sizeof(MemoriaCompartida));
        close(fd_shm);
        shm_unlink(SHM_NAME);
        mq_close(queue);
        mq_unlink(MQ_NAME);
    }

    printf("[%d] Finishing\n", getpid());
    fflush(stdout);
    exit(EXIT_SUCCESS);
}
