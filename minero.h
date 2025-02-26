/**
 * @file minero.h
 * @author Sara Serrano Marazuela
 * @author Xiomara Caballero Cuya
 * @brief Module minero
 * @version 2.0
 * @date 2024-02-01
 *
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
int proceso_minero(int rondas, int hilos, long int objetivo, int fd_escritura, int fd_lectura);

/**
 * @brief search in the function pow_hash a target in a ragne of number
 *
 * @param arg dates to search in Minero
 */
void *busqueda(void *arg);