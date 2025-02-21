#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int proceso_minero(int rondas, int hilos, long int objetivo, int fd_escritura, int fd_lectura);

void *busqueda(void *arg);