#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include "pow.h"
#include "monitor.h"

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
                printf("Solution rejected: %08ld !-> %08ld\n", objetivo, solucion);
                fflush(stdout);
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

int main(int argc, char* argv[]) {

    int fd_shm;
    int lag;
    bool fin = false;
    Block *shm_block;

    if(argc == 2) {
        lag = atoi(argv[1]);
    } else {
        printf("Error en los argumentos del ejecutable minero\n");
        return EXIT_FAILURE;
    }
    if(lag <= 1){
        printf("Error en los argumentos del ejecutable minero\n");
        return EXIT_FAILURE;
    }

    fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if(fd_shm == -1) {
        fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
        if(fd_shm == -1) {
            perror("Error al abrir el segmento de memoria compartida\n");
            return EXIT_FAILURE;
        }
        // MINERO
        close(fd_shm);
        shm_unlink(SHM_NAME);
    } else {
        // COMPROBADOR

        if(ftruncate(fd_shm, sizeof(Block)) == -1) {
            perror("ftruncate");
            close(fd_shm);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        while (fin == false) {
            shm_block = mmap(NULL, sizeof(Block), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
            close(fd_shm);
            if(shm_block == MAP_FAILED) {
                perror("mmap");
                close(fd_shm);
                shm_unlink(SHM_NAME);
                exit(EXIT_FAILURE);
            }

            /*FALTA COMPROBAR CUÁNDO ES VÁLIDO*/

            fin = shm_block->fin;
            usleep(lag);
        }

        close(fd_shm);
    }
    return EXIT_SUCCESS;
}
