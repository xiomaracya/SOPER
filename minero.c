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
int proceso_minero(int rondas, int hilos, long int objetivo){
    int i, j, error, terminar=0;
    pthread_t h[hilos];
    Datos datos [hilos];
    long int intervalo = floor(POW_LIMIT/hilos);
    long int sobran = POW_LIMIT-intervalo*hilos;
    long int inicio=0;
    long int objetivo_ronda = objetivo;

    for (i=0; i<rondas; i++) {
        long int solucion = -1;
        for (j=0; j<sobran; j++){
            datos[j].inicio_rango = inicio;
            inicio+=intervalo+1;
            datos[j].final_rango = inicio-1;
            datos[j].solucion = &solucion;
            datos[j].objetivo = objetivo_ronda;
            error = pthread_create(&h[j], NULL, busqueda, &datos[j]);
            if(error!=0) {
                fprintf(stderr, "pthread_create: %s\n", strerror(error));
                return EXIT_FAILURE;
            }

        }
        for (; j<hilos; j++){
            datos[j].inicio_rango = inicio;
            inicio+=intervalo;
            datos[j].final_rango = inicio-1;
            datos[j].solucion = &solucion;
            error = pthread_create(&h[j], NULL, busqueda, &datos[j]);
            if(error!=0) {
                fprintf(stderr, "pthread_create: %s\n", strerror(error));
                exit(EXIT_FAILURE);
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
        objetivo_ronda = solucion;
    }
    return EXIT_SUCCESS;
}

void *busqueda(void *arg){
    Datos *args = (Datos*)arg;
    int i;

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