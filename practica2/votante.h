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
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "principal.h"
#include <semaphore.h>
#include <errno.h>

#ifndef VOTANTE_H
#define VOTANTE_H

/**
 *  Función que va a leer del fichero los pids
 * 
 *  @param fd identificador del fichero
 *  @param nProc puntero al núemro de procesos votante
 *  @return un array con los pids
 */
int *readpids(int fd, int *nProc);

/**
 *  Función que va a leer del fichero los votos
 * 
 *  @param fd identificador del fichero
 *  @param nProc puntero al núemro de procesos votante
 *  @param ronda el número de ronda que se está ejecutando
 *  @return un array con los votos
 */
char *readvotes(int fd, int *nProc, int *ronda);

/**
 *  Función que va a leer del fichero los votos
 * 
 *  @param fd identificador del fichero
 *  @param sem1 semáforo 1 usado para identificar el proceso candidato y los votantes
 *  @param sem2 semáforo 2 usado para que los procesos no escriban en el fichero a la vez
 *  @param sem3 semáforo 3 usado para saber cuándo todos los procesos votante han terminado de votar y escribir en el fichero
 *  @param nProc número de procesos votantes
 *  @param mask la máscara con las señales bloqueadas a utilizar
 *  @param ronda el número de ronda que se está ejecutando
 *  @return el estado tras el procedimiento
 */
int chooseCandidato(int fd, sem_t *sem1, sem_t *sem2, sem_t *sem3, int nProc, sigset_t mask, int *ronda);

/**
 *  Función que va a organizar a un votante o candidato
 * 
 *  @param fd identificador del fichero
 *  @param sem1 semáforo 1 usado para identificar el proceso candidato y los votantes
 *  @param sem2 semáforo 2 usado para que los procesos no escriban en el fichero a la vez
 *  @param sem3 semáforo 3 usado para saber cuándo todos los procesos votante han terminado de votar y escribir en el fichero
 *  @param nProc número de procesos votantes
 *  @param mask la máscara con las señales bloqueadas a utilizar
 *  @return el estado tras el procedimiento
 */
int votante(int fd, sem_t *sem1, sem_t *sem2, sem_t *sem3, int nProc, sigset_t mask);

#endif