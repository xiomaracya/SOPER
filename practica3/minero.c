#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "pow.h"
#include "minero.h"
#include "monitor.h"
#include <mqueue.h>


typedef struct {
    long int inicio_rango;
    long int final_rango;
    long int *solucion;
    long int objetivo;
} Datos;

int proceso_minero(int rondas, int hilos, long int objetivo, mqd_t mq, int lag){
    int i, j, error;
    pthread_t h[hilos];
    Datos datos [hilos];
    long int intervalo = floor((POW_LIMIT+1)/hilos);
    long int sobran = POW_LIMIT-intervalo*hilos;
    long int objetivo_ronda = objetivo;
    long int solucion;
    long int inicio;
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
        
        Block *b = malloc(sizeof(Block));
        b->objetivo = objetivo;
        b->solucion = solucion;
        b->fin = false;

        if (mq_send(mq, (const char *)&b, sizeof(Block), 0) == -1) {
            perror("mq_send");
            break;
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

        usleep(lag*1000);
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

int main(int argc, char* argv[]) {
    int rondas = 0, lag = 0;
    int obj_inicial = 0;
    if(argc == 3) {
        rondas = atoi(argv[1]);
        lag = atoi(argv[2]);
    } else {
        printf("Error en los argumentos del ejecutable minero\n");
        return EXIT_FAILURE;
    }

    if(rondas<=0 || lag<0) {
        printf("Error en los argumentos del ejecutable minero\n");
        return EXIT_FAILURE;
    }

    // MINERO
    //Se crea la cola de mensajes
    struct mq_attr attributes;
    mqd_t mq;
    attributes.mq_flags = 0;
    attributes.mq_maxmsg = MAX_MSG;
    attributes.mq_msgsize = sizeof(Block);
    attributes.mq_curmsgs = 0;

    mq_unlink(MQ_NAME);
    mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return EXIT_FAILURE;
    }
    //realiza el proceso de resolver el pow

    proceso_minero(rondas, MAX_THREADS, obj_inicial, mq, lag);

    //Mandar mensaje para que comprobador recbia q ha terminado
    Block final_bloque = { .objetivo = -1, .solucion = -1, .fin = true };

    mq_send(mq, (const char *)&final_bloque, sizeof(Block), 0);

    //Finalizamos y liberamos recursos
    mq_close(mq);
    mq_unlink(MQ_NAME);

    return EXIT_SUCCESS;
}


