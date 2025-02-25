#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pow.h"

int monitor(int fd_escritura, int fd_lectura) {
    int nbytes = 0;
    long int solucion;
    long int objetivo;
    char solucion_char[8];
    char objetivo_char[8];
    char retorno [6];

    do {
        nbytes = read(fd_lectura, objetivo_char, sizeof(objetivo_char));
        if (nbytes == -1) {
            perror("read");
            return EXIT_FAILURE;
        }

        nbytes = read(fd_lectura, solucion_char, sizeof(solucion_char));
        if (nbytes == -1) {
            perror("read");
            return EXIT_FAILURE;
        }

        if (nbytes>0) {

            solucion = atol(solucion_char);
            objetivo = atol(objetivo_char);

            if(pow_hash(solucion) == objetivo) {
                printf("Solution accepted: %08ld --> %08ld\n", objetivo, solucion);
                fflush(stdout);
                strcpy(retorno, "OK");
            } else {
                printf("Solution rejected: %08ld ! --> %08ld\n", objetivo, solucion);
                strcpy(retorno, "ERROR");
            }

            nbytes = write(fd_escritura, retorno, sizeof(retorno));
            if (nbytes == -1) {
                perror("write");
                return EXIT_FAILURE;
            }
        }
    } while (nbytes>0);


    if(nbytes==0) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }

}
