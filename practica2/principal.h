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
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PID 30
#define FICHERO "PIDS.txt"

#ifndef PRINCIPAL_H
#define PRINCIPAL_H

/**
 *  Función handle encargada de controlar la señal sigint
 * 
 *  @param sig número de señal que ha sido recibida por el proceso
 */
void handle_sigint(int sig);

/**
 *  Función handle encargada de controlar la señal sigterm
 * 
 *  @param sig número de señal que ha sido recibida por el proceso
 */
void handle_sigterm(int sig);

/**
 *  Función handle encargada de controlar la señal sigalarm
 * 
 *  @param sig número de señal que ha sido recibida por el proceso
 */
void handle_sigalarm(int sig);


/**
 *  Función handle encargada de controlar la señal sigusr1
 * 
 *  @param sig número de señal que ha sido recibida por el proceso
 */
void handle_sigusr1(int sig);

/**
 *  Función handle encargada de controlar la señal sigusr2
 * 
 *  @param sig número de señal que ha sido recibida por el proceso
 */
void handle_sigusr2(int sig);

#endif