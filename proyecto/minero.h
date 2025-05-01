/**
 * @file minero.h
 * @author Sara Serrano Marazuela
 * @author Xiomara Caballero Cuya
 * @brief Module minero
 * @version 1.0
 * @date 2025-02-01
 *
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include "monitor.h"

#ifndef _MINERO_H
#define _MINERO_H

/**
 * Macro del tamaño de la cola de mensane
 */
#define MAX_MSG 7
/**
 * Macro del nombre de la cola de mensane
 */
#define MQ_NAME "/mq"
/**
 * Macro del numero de hilos utilizamos en minero
 */
#define MAX_THREADS 7

/**
 * Macro del número máximo de procesos mineros
 */
#define SHM_SISTEMA "/sistema_shm" /*< Nombre del segmento de memoria compartida */

/**
 * @struct Sistema
 * @brief Estructura que representa al sistema
 */
typedef struct {
    pid_t pid[MAX_MINEROS]; /*< pids de los mineros que se van añadiendo */
    int votos[MAX_MINEROS]; /*< el voto de cada uno de los procesos minero */
    int monedas[MAX_MINEROS]; /*< monedas de cada uno de los mineros */

    sem_t sem_ganador; /*< Semáforo para asegurar que solo hay un ganador */

    Block ultimo_bloque; /*< último bloque resuelto */
    Block bloque_actual; /*< bloque actual */

    int in; /*< Índice para escribir */
    int out; /*< Índice para leer */
    sem_t sem_empty; /*< Semáforo para contar los espacios vacíos */
    sem_t sem_full; /*< Semáforo para contar los espacios llenos */
    sem_t sem_mutex; /*< Semáforo para saber si se puede escribir en las variables y arrays */
    sem_t sem_mutex_bloque; /*< Semáforo para saber si se puede escribir en las variables y arrays de los bloques */

} Sistema;

/**
 * @brief Minates a function with a number of thread and rounds
 *
 * @param rondas number of rounds to search a target
 * @param hilos number of thread to use
 * @param objetivo solution to be searched
 * @param fd_escritura file descriptor that allows to write in the first pipe
 * @param fd_lectura file descriptor that allows to read in the second pipe
 * @return status of the operation EXIT_FAILURE or EXIT_SUCCESS
 */
int proceso_minero(int hilos, Sistema *shm_sistema, mqd_t mq);

/**
 * @brief search in the function pow_hash a target in a ragne of number
 *
 * @param arg dates to search in Minero
 */
void *busqueda(void *arg);

#endif