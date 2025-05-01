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

#ifndef _MONITOR_H
#define _MONITOR_H


#define MAX_BLOCKS 6 /*!< Número máximo de bloques en el buffer */
#define MAX_PID 30 /*< Número máximo de procesos */
#define SHM_NAME "/minero_shm" /*< Nombre del segmento de memoria compartida */

/**
 * Macro del número máximo de procesos mineros
 */
#define MAX_MINEROS 100

/**
 * @struct Block
 * @brief Estructura que representa un bloque de datos
 */
typedef struct {
    long int id; /*< Identificador del bloque */
    long int objetivo; /*< Objetivo introducido */
    long int solucion; /*< Solución encontrada */
    pid_t ganador; /*< PID del minero ganador */

    int num_carteras; /*< Número de mineros activos */
    pid_t pid_carteras[MAX_MINEROS]; /*< PIDs de los mineros */
    int monedas[MAX_MINEROS]; /*< carteras actuales de los mineros */

    int num_votos_totales; /*< Número de votos totales que se han usado para evaluar este bloque */
    int num_votos_positivos; /*< Número de votos positivos para este bloque */
    bool flag; /*< Bandera que indica si se finaliza */
    bool fin; /*< Estado de la solución */
} Block;

/**
 * @struct MemoriaCompartida
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

#endif