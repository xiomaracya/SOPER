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

/**
 * @brief Verify the solution found by Minero
 *
 * @param fd_escritura file descriptor that allows to write in the second pipe
 * @param fd_lectura file descriptor that allows to read in the first pipe
 */
int monitor(int fd_escritura, int fd_lectura);