/**
 * @file monitor.h
 * @author Sara Serrano Marazuela
 * @author Xiomara Caballero Cuya
 * @brief Module monitor
 * @version 2.0
 * @date 2024-02-01
 *
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>


#define MAX_BLOCKS 6
#define MAX_PID 30
#define SHM_NAME "/minero_shm"
#define MQ_NAME "/mq"

typedef struct {
    long int objetivo;
    long int solucion;
    bool flag;
    bool fin;
} Block;

typedef struct {
    Block buffer[MAX_BLOCKS];
    int in;
    int out;
    sem_t sem_empty;
    sem_t sem_full;
    sem_t sem_mutex;
} MemoriaCompartida;

/**
 * @brief Verify the solution found by Minero
 *
 * @param fd_escritura file descriptor that allows to write in the second pipe
 * @param fd_lectura file descriptor that allows to read in the first pipe
 */
int monitor(int fd_escritura, int fd_lectura);