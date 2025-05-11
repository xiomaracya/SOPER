/**
 * @file minero.c
 * @author Sara Serrano Marazuela
 * @author Xiomara Caballero Cuya
 * @brief Module minero
 * @version 1.0
 * @date 2025-04-15
 *
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include "pow.h"
#include "minero.h"
#include "monitor.h"
#include <mqueue.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/wait.h>

static volatile sig_atomic_t finalizar = 0;
static volatile sig_atomic_t fin_votacion = 0;

void handle_sigint(int sig) {
    (void)sig;
    finalizar = 1;
}

void handle_sigalarm(int sig) {
    (void)sig;
    finalizar = 1;
}

void handle_sigusr1(int sig) {
    (void)sig;
}

void handle_sigusr2(int sig) {
    (void)sig;
    fin_votacion = 1;
}


/**
 * @struct Datos
 * @brief Estructura que representa los datos necesarios para la búsqueda
 */
typedef struct {
    long int inicio_rango;
    long int final_rango;
    long int *solucion;
    long int objetivo;
} Datos;

/**
 * @brief El proceso minero que ejecuta los bloques
 *
 * @param rondas el número de rondas a realizar
 * @param hilos el número de hilos a utilizar
 * @param objetivo el objetivo a buscar
 * @param mq la cola de mensajes para enviar los bloques
 * @param lag el retardo entre rondas
 */
int proceso_minero(int hilos, Sistema *shm_sistema, mqd_t mq) {
    int j, error;
    pthread_t h[hilos];
    Datos datos [hilos];
    long int intervalo = floor((POW_LIMIT+1)/hilos);
    long int sobran = POW_LIMIT-intervalo*hilos;
    long int solucion;
    long int inicio;
    long int objetivo_ronda;

    sem_wait(&shm_sistema->sem_mutex_bloque);
    objetivo_ronda = shm_sistema->bloque_actual.objetivo;
    sem_post(&shm_sistema->sem_mutex_bloque);

    solucion = -1;
    inicio=0;

    fin_votacion = 0;
    /* SE UTILIZAN MÚLTIPLES HILOS EN PARALELO */
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

        /* SE CREA EL HILO QUE TOCA */
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

    if(sem_trywait(&shm_sistema->sem_ganador) == -1) {
        /* PROCESO PERDEDOR */

        /* EL PROCESO PERDEDOR VOTA*/
        sem_wait(&shm_sistema->sem_mutex_bloque);
        sem_wait(&shm_sistema->sem_mutex);
        for (int i = 0; i < MAX_MINEROS; i++) {
            if(shm_sistema->pid[i] == getpid()) {
                if (pow_hash(shm_sistema->bloque_actual.solucion) == shm_sistema->bloque_actual.objetivo) {
                    shm_sistema->bloque_actual.num_votos_positivos++;
                    shm_sistema->votos[i] = 1;
                } else {
                    shm_sistema->votos[i] = 0;
                }
                break;
            }
        }
        shm_sistema->bloque_actual.num_votos_totales++;
        sem_post(&shm_sistema->sem_mutex);
        sem_post(&shm_sistema->sem_mutex_bloque);

    } else {
        /* PROCESO GANADOR */

        /* SE ENVÍA LA SEÑAL USR2 */
        sem_wait(&shm_sistema->sem_mutex_bloque);
        sem_wait(&shm_sistema->sem_mutex);

        for (int i = 0; i < MAX_MINEROS; i++) {
            if(shm_sistema->bloque_actual.pid_carteras != 0 && shm_sistema->bloque_actual.pid_carteras[i] != getpid()) {
                kill(shm_sistema->bloque_actual.pid_carteras[i], SIGUSR2);
            }
        }

        shm_sistema->bloque_actual.id = shm_sistema->ultimo_bloque.id + 1;
        shm_sistema->bloque_actual.objetivo = objetivo_ronda;

        shm_sistema->bloque_actual.solucion = solucion;
        shm_sistema->bloque_actual.ganador = getpid();
        for (int i = 0; i < MAX_MINEROS; i++) {
            shm_sistema->bloque_actual.monedas[i] = shm_sistema->monedas[i];
        }
        shm_sistema->bloque_actual.flag = false;
        shm_sistema->bloque_actual.fin = false;
        shm_sistema->bloque_actual.num_votos_totales = 0;
        shm_sistema-> bloque_actual.num_votos_positivos = 0;

        sem_post(&shm_sistema->sem_mutex);
        sem_post(&shm_sistema->sem_mutex_bloque);

        /* VOTA EL MINERO GANADOR */
        sem_wait(&shm_sistema->sem_mutex_bloque);
        sem_wait(&shm_sistema->sem_mutex);
        shm_sistema->bloque_actual.num_votos_totales++;
        for (int i = 0; i < MAX_MINEROS; i++) {
            if(shm_sistema->pid[i] == getpid()) {
                if (pow_hash(shm_sistema->bloque_actual.solucion) == shm_sistema->bloque_actual.objetivo) {
                    shm_sistema->bloque_actual.num_votos_positivos++;
                    shm_sistema->votos[i] = 1;
                } else {
                    shm_sistema->votos[i] = 0;
                }
                break;
            }
        }
        sem_post(&shm_sistema->sem_mutex);
        sem_post(&shm_sistema->sem_mutex_bloque);

        /* ESPERAR HASTA QUE TODOS LOS MINEROS HAYAN VOTADO */
        int intentos = 0;
        while (intentos<1000) {
            sem_wait(&shm_sistema->sem_mutex_bloque);
            if (shm_sistema->bloque_actual.num_votos_totales >= shm_sistema->bloque_actual.num_carteras) {
                sem_post(&shm_sistema->sem_mutex_bloque);
                break;
            }
            sem_post(&shm_sistema->sem_mutex_bloque);
            usleep(100);
            intentos++;
        }

        sem_wait(&shm_sistema->sem_mutex_bloque);
        sem_wait(&shm_sistema->sem_mutex);

        /* COMPROBAR SI SE APRUEBA EL BLOQUE */
        if (shm_sistema->bloque_actual.num_votos_positivos > shm_sistema->bloque_actual.num_carteras / 2) {
            shm_sistema->bloque_actual.flag = true;
            for (int i = 0; i < MAX_MINEROS; i++) {
                if (shm_sistema->bloque_actual.pid_carteras[i] == getpid()) {
                    shm_sistema->bloque_actual.monedas[i]++;
                    shm_sistema->monedas[i]++;
                    break;
                }
            }
        }

        sem_post(&shm_sistema->sem_mutex);
        sem_post(&shm_sistema->sem_mutex_bloque);

        intentos = 0;
        while (mq_send(mq, (char*)&shm_sistema->bloque_actual, sizeof(Block), 0) == -1) {
            if (errno == EINTR && intentos < 1000) {
                intentos++;
                usleep(1000); // 1ms
                continue;
            }
            perror("mq_send_ganador");
            return EXIT_FAILURE;
        }


        sem_wait(&shm_sistema->sem_mutex_bloque);
        sem_wait(&shm_sistema->sem_mutex);

        shm_sistema->ultimo_bloque = shm_sistema->bloque_actual;
        Block bloque_actual;
        memset(&bloque_actual, 0, sizeof(Block));

        if (shm_sistema->bloque_actual.flag) {
            bloque_actual.objetivo = solucion;
        } else {
            bloque_actual.objetivo = shm_sistema->bloque_actual.objetivo;
        }

        bloque_actual.solucion = 0;
        bloque_actual.num_carteras = 0;
        bloque_actual.num_votos_totales = 0;
        bloque_actual.num_votos_positivos = 0;

        for (int i = 0; i < MAX_MINEROS; i++) {
            bloque_actual.pid_carteras[i] = 0;
            bloque_actual.monedas[i] = 0;
        }

        shm_sistema->bloque_actual = bloque_actual;

        sem_post(&shm_sistema->sem_mutex_bloque);

        for (int i = 0; i < MAX_MINEROS; i++) {
            pid_t pid = shm_sistema->pid[i];
            if (pid != 0) {
                if (kill(pid, SIGUSR1) == -1) {
                    perror("kill SIGUSR1");
                }
            }
        }
        sem_post(&shm_sistema->sem_mutex);

        sem_post(&shm_sistema->sem_ganador);
        
    }
    return EXIT_SUCCESS;
}

/**
 * @brief The function that is going to solve pow_hash
 * 
 * @param arg pointer to the struct Datos
 */
void *busqueda(void *arg){
    Datos *args = (Datos*)arg;
    long int i;

    for (i = args->inicio_rango; i<=args->final_rango && *args->solucion == -1 && fin_votacion == 0; i++){
        if(args->objetivo==pow_hash(i)){
            *args->solucion = i;
            return NULL;
        }
    }
    return NULL;   
}

/**
 * @brief The main that is going to execute miner
 *
 * @param argc número de argumentos
 * @param argv argumentos
 */
int main(int argc, char* argv[]) {
    int n_seconds = 0, n_threads = 0;
    struct sigaction act_int, act_alarm, act_usr1, act_usr2;
    int fd_shm;
    Sistema *shm_sistema = NULL;
    struct mq_attr attributes;
    mqd_t mq;
    int sig;

    int pipefd[2];
    if(pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t registrador_pid = fork();
    if(registrador_pid<0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (registrador_pid == 0) {
        close(pipefd[1]);

        char filename[64];
        snprintf(filename, sizeof(filename), "registro_%d.txt", getppid());
        int fd_reg = open(filename, O_WRONLY | O_CREAT | O_APPEND | O_SYNC, 0644);
        if(fd_reg == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        Block b;
        ssize_t bytes_read;
        int exit_code = EXIT_SUCCESS;

        while ((bytes_read = read(pipefd[0], &b, sizeof(Block))) > 0) {
            
            if(bytes_read == -1) {
                if(errno == EINTR) continue;
                perror("Error leyendo de la tubería");
                exit_code = EXIT_FAILURE;
                break;
            }
            if(bytes_read != sizeof(Block)) {
                fprintf(stderr, "Lectura incompleta del bloque\n");
                break;
            }
            printf("Registrador recibió bloque %ld\n", b.id);
            fflush(stdout);
            if(b.flag) {
                dprintf(fd_reg, "ID: %04ld\n", b.id);
                if(dprintf(fd_reg, "ID: %04ld\n", b.id) < 0) {
                    perror("Error escribiendo en registro");
                }
                dprintf(fd_reg, "Ganador: %d\n", b.ganador);
                dprintf(fd_reg, "Objetivo: %08ld\n", b.objetivo);
                dprintf(fd_reg, "Solución: %08ld\n", b.solucion);
                dprintf(fd_reg, "Votos: %d/%d\n", b.num_votos_positivos, b.num_votos_totales);
                dprintf(fd_reg, "Wallets:");
                
                for (int i = 0; i < MAX_MINEROS; i++) {
                    if (b.pid_carteras[i] != 0) {
                        dprintf(fd_reg, " %d:%d", b.pid_carteras[i], b.monedas[i]);
                    }
                }
                
                dprintf(fd_reg, "\n--------------------------\n");
                if(fsync(fd_reg) == -1) {
                    perror("Error sincronizando archivo");
                    exit_code = EXIT_FAILURE;
                    break;
                }
            }
            
        }

        close(fd_reg);
        close(pipefd[0]);
        exit(exit_code);
    } else {
        close(pipefd[0]);

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);
        sigaddset(&mask, SIGINT);
        pthread_sigmask(SIG_BLOCK, &mask, NULL);

        /* CAPTURA DE SIGINT */
        act_int.sa_handler = handle_sigint;
        sigemptyset(&(act_int.sa_mask));
        act_int.sa_flags = 0;

        if(sigaction(SIGINT, &act_int, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        /* CAPTURA DE SIGALARM */
        act_alarm.sa_handler = handle_sigalarm;
        sigemptyset(&(act_alarm.sa_mask));
        act_alarm.sa_flags = 0;

        if(sigaction(SIGALRM, &act_alarm, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        /* CAPTURA DE SIGUSR1 */
        act_usr1.sa_handler = handle_sigusr1;
        sigemptyset(&(act_usr1.sa_mask));
        act_usr1.sa_flags = 0;

        if(sigaction(SIGUSR1, &act_usr1, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        /* CAPTURA DE SIGUSR2 */
        act_usr2.sa_handler = handle_sigusr2;
        sigemptyset(&(act_usr2.sa_mask));
        act_usr2.sa_flags = 0;

        if(sigaction(SIGUSR2, &act_usr2, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        /* COMPROBACIÓN DE ARGUMENTOS */

        if(argc == 3) {
            n_seconds = atoi(argv[1]);
            n_threads = atoi(argv[2]);
        } else {
            printf("Error en los argumentos del ejecutable minero\n");
            exit(EXIT_FAILURE);
        }

        if(n_seconds<=0 || n_threads<0) {
            printf("Error en los argumentos del ejecutable minero\n");
            exit(EXIT_FAILURE);
        }

        /* ALARMA PARA QUE EL PROCESO TERMINE SI PASAN LOS SEGUNDOS ESPECIFICADOS */
        alarm(n_seconds);
        /* COMPRUEBA SI SE HA CREADO YA EL SISTEMA O NO */
        fd_shm = shm_open(SHM_SISTEMA, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if(fd_shm == -1) {
            /* SE AÑADE UN MINERO AL SISTEMA */

            /* SE ABRE UN SEGMENTO DE MEMORIA COMPARTIDA */
            while((fd_shm = shm_open(SHM_SISTEMA, O_RDWR, 0)) == -1) {
                usleep(1000);
            }

            shm_sistema = mmap(NULL, sizeof(Sistema), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
            if(shm_sistema == MAP_FAILED) {
                perror("mmap");
                close(fd_shm);
                shm_unlink(SHM_SISTEMA);
                exit(EXIT_FAILURE);
            }

            /* SE CREA LA COLA DE MENSAJES */
            while ((mq = mq_open(MQ_NAME, O_WRONLY)) == (mqd_t)-1) {
                if (errno == ENOENT) {
                    usleep(1000); // espera a que el primero la cree
                } else {
                    perror("mq_open_minero");
                    exit(EXIT_FAILURE);
                }
            }

            /* ESPERA A QUE EL ÚLTIMO SEMÁFORO ESTÉ CREADO */

            while(sem_trywait(&shm_sistema->sem_mutex) == -1) {
                if(errno == EAGAIN) {
                    usleep(100);
                } else {
                    perror("sem_trywait");
                    munmap(shm_sistema, sizeof(Sistema));
                    close(fd_shm);
                    shm_unlink(SHM_SISTEMA);
                    exit(EXIT_FAILURE);
                }
            }
            sem_post(&shm_sistema->sem_mutex);

            if(sem_trywait(&shm_sistema->sem_empty) == -1) {
                munmap(shm_sistema, sizeof(Sistema));
                close(fd_shm);
                shm_unlink(SHM_SISTEMA);
                exit(EXIT_FAILURE);
            }

            /* SE AÑADE EL NUEVO PID AL SISTEMA */
            sem_wait(&shm_sistema->sem_empty);
            sem_wait(&shm_sistema->sem_mutex);
            for (int i = 0; i < MAX_MINEROS; i++) {
                if (shm_sistema->pid[i] == 0) {
                    shm_sistema->pid[i] = getpid();
                    shm_sistema->votos[i] = 0;
                    break;
                }
            }

            sem_post(&shm_sistema->sem_mutex);
            sem_post(&shm_sistema->sem_full);

        } else {
            /* SE CREA SISTEMA POR PRIMERA VEZ */

            /* SE ESTABLECE EL TAMAÑO DEL SEGMENTO DE MEMORIA COMPARTIDA */
            if(ftruncate(fd_shm, sizeof(Sistema)) == -1) {
                perror("ftruncate");
                close(fd_shm);
                shm_unlink(SHM_SISTEMA);
                exit(EXIT_FAILURE);
            }

            /* HACE UN MAPEO DE LA MEMORIA COMPARTIDA */
            shm_sistema = mmap(NULL, sizeof(Sistema), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
            if(shm_sistema == MAP_FAILED) {
                perror("mmap");
                close(fd_shm);
                shm_unlink(SHM_SISTEMA);
                exit(EXIT_FAILURE);
            }

            Block bloque_actual;
            memset(&bloque_actual, 0, sizeof(Block));
            bloque_actual.objetivo = 0;
            bloque_actual.solucion = 0;
            bloque_actual.num_carteras = 0;
            for (int i = 0; i < MAX_MINEROS; i++) {
                bloque_actual.pid_carteras[i] = 0;
                bloque_actual.monedas[i] = 0;
            }

            Block ultimo_bloque;
            memset(&ultimo_bloque, 0, sizeof(Block));
            ultimo_bloque.id = -1;
            ultimo_bloque.num_carteras = 0;

            shm_sistema->bloque_actual = bloque_actual;
            shm_sistema->ultimo_bloque = ultimo_bloque;

            /* SE CREA LA COLA DE MENSAJES */
            attributes.mq_flags = 0;
            attributes.mq_maxmsg = MAX_MSG;
            attributes.mq_msgsize = sizeof(Block);
            attributes.mq_curmsgs = 0;

            mq_unlink(MQ_NAME);
            mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
            if (mq == (mqd_t)-1) {
                perror("mq_open_crear");
                exit(EXIT_FAILURE);
            }

            /* INICIALIZA LOS ARRAYS */
            for (int i = 0; i < MAX_MINEROS; i++) {
                shm_sistema->pid[i] = 0;
                shm_sistema->monedas[i] = 0;
                shm_sistema->votos[i] = 0;
            }

            /* INICIALIZA LOS SEMÁFOROS */
            shm_sistema->in = 0;
            shm_sistema->out = 0;
            sem_init(&shm_sistema->sem_empty, 1, MAX_MINEROS);
            sem_init(&shm_sistema->sem_full, 1, 0);
            sem_init(&shm_sistema->sem_mutex, 1, 1);
            sem_init(&shm_sistema->sem_mutex_bloque, 1, 1);
            sem_init(&shm_sistema->sem_ganador, 1, 1);

            sem_wait(&shm_sistema->sem_empty);
            sem_wait(&shm_sistema->sem_mutex);
            /* SE AÑADE EL NUEVO PID AL SISTEMA */
            for (int i = 0; i < MAX_MINEROS; i++) {
                if (shm_sistema->pid[i] == 0) {
                    shm_sistema->pid[i] = getpid();
                    shm_sistema->votos[i] = 0;
                    break;
                }
            }

            sem_post(&shm_sistema->sem_mutex);
            sem_post(&shm_sistema->sem_full);

            /* SE ENVÍA LA SEÑAL SIGUSR1 A LOS PROCESOS MINERO QUE YA ESTÁN ESPERANDO */

            sem_wait(&shm_sistema->sem_mutex);
            for (int i = 0; i < MAX_MINEROS; i++) {
                if(shm_sistema->pid[i] != 0) {
                    if(kill(shm_sistema->pid[i], SIGUSR1)) {
                        perror("kill");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            sem_post(&shm_sistema->sem_mutex);
            
        }

        /* REALIZA EL PROCESO PARA RESOLVER EL POW */

        while(!finalizar) {
            if (sigwait(&mask, &sig) != 0) {
                perror("sigwait"); 
                return EXIT_FAILURE;
            }

            sem_wait(&shm_sistema->sem_mutex_bloque);
            sem_wait(&shm_sistema->sem_mutex);

            if(shm_sistema->ultimo_bloque.id != -1) {
                Block b;
                memcpy(&b, &shm_sistema->ultimo_bloque, sizeof(Block));
                if (write(pipefd[1], &b, sizeof(Block)) == -1) {
                    perror("write pipe");
                }
            }
            shm_sistema->bloque_actual.num_carteras++;
            for (int i = 0; i < MAX_MINEROS; i++) {
                if(shm_sistema->pid[i] == getpid()) {
                    shm_sistema->bloque_actual.pid_carteras[i] = getpid();
                    shm_sistema->bloque_actual.monedas[i] = shm_sistema->monedas[i];
                    break;
                }
            }
            sem_post(&shm_sistema->sem_mutex);
            sem_post(&shm_sistema->sem_mutex_bloque);

            proceso_minero(n_threads, shm_sistema, mq);
        }
        

        /* FINALIZA EL PROCESO Y LIBERA RECURSOS */
        close(pipefd[1]); // Cierra escritura en padre
        waitpid(registrador_pid, NULL, 0);
        sem_wait(&shm_sistema->sem_mutex);
        int total_pids = 0;

        for (int i = 0; i < MAX_MINEROS; i++) {
            if(shm_sistema->pid[i] != 0) {
                total_pids++;
            }
            if(shm_sistema->pid[i] == getpid()) {
                shm_sistema->pid[i] = 0;
                shm_sistema->monedas[i] = 0;
                shm_sistema->votos[i] = 0;
            }
        }

        sem_post(&shm_sistema->sem_mutex);

        if(total_pids == 1) {
            Block mensaje;
            memset(&mensaje, 0, sizeof(Block));
            mensaje.flag = false;
            mensaje.fin = true;

            if (mq_send(mq,(char*)&mensaje, sizeof(Block), 0) == -1) {
                perror("mq_send");
                exit(EXIT_FAILURE);
            }

            mq_close(mq);
            munmap(shm_sistema, sizeof(Sistema));
            close(fd_shm);
            shm_unlink(SHM_SISTEMA);
            exit(EXIT_SUCCESS);
        }

        sem_wait(&shm_sistema->sem_full);
        sem_post(&shm_sistema->sem_empty);
        exit(EXIT_SUCCESS);
    }
}


