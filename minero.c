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
    long int solucion;
    long int inicio;
    char objetivo_char[8];
    char solucion_char[8];
    char retorno[6];

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
        sprintf(objetivo_char, "%ld", objetivo_ronda);
        sprintf(solucion_char, "%ld", solucion);
        nbytes = write(fd_escritura, objetivo_char, sizeof(objetivo_char));
        if (nbytes == -1) {
            perror("write");
            return EXIT_FAILURE;
        }
        nbytes = write(fd_escritura, solucion_char, sizeof(solucion_char));
        if (nbytes == -1) {
            perror("write");
            return EXIT_FAILURE;
        }
        nbytes = read(fd_lectura, retorno, sizeof(retorno));
        if (nbytes == -1) {
            perror("read");
            return EXIT_FAILURE;
        }
        if(pow_hash(solucion) == objetivo_ronda) {
            if(strcmp(retorno, "OK")) {
                printf("The solution has been invalidated\n");
                return EXIT_FAILURE;
            }
        } else {
            if(strcmp(retorno, "ERROR")) {
                printf("The solution has been invalidated\n");
                return EXIT_FAILURE;
            }
            return EXIT_FAILURE;
        }
        objetivo_ronda = solucion;
    }
    return EXIT_SUCCESS;
}

void *busqueda(void *arg){
    Datos *args = (Datos*)arg;
    long int i;

    for (i = args->inicio_rango; i<=args->final_rango && *args->solucion == -1; i++){
        if(args->objetivo==pow_hash(i)){
            *args->solucion = i;
            return NULL;
        }
    }
    return NULL;
    
}