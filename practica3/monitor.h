/**
 * @file monitor.h
 * @author Sara Serrano Marazuela
 * @author Xiomara Caballero Cuya
 * @brief Module monitor
 * @version 1.0
 * @date 2025-02-01
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


#define MAX_BLOCKS 6 /*!< Número máximo de bloques en el buffer */
#define MAX_PID 30 /*< Número máximo de procesos */
#define SHM_NAME "/minero_shm" /*< Nombre del segmento de memoria compartida */

/**
 * @struct Block
 * @brief Estructura que representa un bloque de datos
 */
typedef struct {
    long int objetivo; /*< Objetivo introducido */
    long int solucion; /*< Solución encontrada */
    bool flag; /*< Bandera que indica si se finaliza */
    bool fin; /*< Estado de la solución */
} Block;

/**
 * @struct Block
 * @brief Estructura que representa la memoria compartida
 */
typedef struct {
    Block buffer[MAX_BLOCKS]; /*< Buffer compartido */
    int in; /*< Índice para escribir */
    int out; /*< Índice para leer */
    sem_t sem_empty; /*< Semáforo para contar los espacios vacíos en el buffer */
    sem_t sem_full; /*< Semáforo para contar los espacios llenos en el buffer */
    sem_t sem_mutex; /*< Semáforo para saber si se puede escribir en el buffer */
} MemoriaCompartida;

/**
 * @brief Verify the solution found by Minero
 *
 * @param fd_escritura file descriptor that allows to write in the second pipe
 * @param fd_lectura file descriptor that allows to read in the first pipe
 */
int monitor(int fd_escritura, int fd_lectura);