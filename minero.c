#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "pow.h"
#include "minero.h"

typedef struct {
    long int inicio_rango;
    long int final_rango;
    long int *solucion;
    long int objetivo;
} Datos;

int proceso_minero(int rondas, int hilos, long int objetivo, int fd_escritura, int fd_lectura){
    int i, j, error;
    int nbytes;
    pthread_t h[hilos];
    Datos datos [hilos];
    long int intervalo = floor((POW_LIMIT+1)/hilos);
    long int sobran = POW_LIMIT-intervalo*hilos;
    long int objetivo_ronda = objetivo;
    long int solucion = -1;
    long int inicio=0;
    char objetivo_char[8];
    char solucion_char[8];

    for (i=0; i<rondas; i++) {
        solucion = -1;
        inicio=0;
        for (j=0; j<hilos; j++){
            datos[j].inicio_rango = inicio;
            if(j<sobran) {
                inicio+=intervalo+1;
            } else {
                inicio+=intervalo;
            }
            datos[j].final_rango = inicio-1;
            datos[j].solucion = &solucion;
            datos[j].objetivo = objetivo_ronda;
            error = pthread_create(&h[j], NULL, busqueda, &datos[j]);
            if(error!=0) {
                fprintf(stderr, "pthread_create: %s\n", strerror(error));
                return EXIT_FAILURE;
            }

        }
        for (j=0; j<hilos; j++){
            error = pthread_join(h[j], NULL);
            if(error!=0) {
                fprintf(stderr, "pthread_join: %s\n", strerror(error));
                return EXIT_FAILURE;
            }
        }
        printf("El valor para la ronda %d es %ld\n", i, solucion);
        sprintf(objetivo_char, "%ld", objetivo_ronda);
        sprintf(solucion_char, "%ld", solucion);
        nbytes = write(fd_escritura, objetivo_char, sizeof(objetivo_char));
        if (nbytes == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        nbytes = write(fd_escritura, solucion_char, sizeof(solucion_char));
        if (nbytes == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        objetivo_ronda = solucion;
    }
    return EXIT_SUCCESS;
}

void *busqueda(void *arg){
    Datos *args = (Datos*)arg;
    long int i;

    i = args->inicio_rango;
    while (i<=args->final_rango && *args->solucion == -1){
        if(args->objetivo==pow_hash(i)){
            *args->solucion = i;
            return NULL;
        }
        i++;
    }
    return NULL;
    
}