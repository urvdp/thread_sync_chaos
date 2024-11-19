#define _POSIX_C_SOURCE 200112L
//#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <pthread.h>
#include <semaphore.h>
#include <ncurses.h>

#include "vehiculo.h"
#include "cola.h"

#define NUM_VEHICULOS 100

bool debug = false;

sem_t sem;
pthread_t hilos[NUM_VEHICULOS];
bool turno_este_oeste = true; // flag para que las posibilidades en ambas colas de espera sean iguales
pthread_mutex_t mutex_turno = PTHREAD_MUTEX_INITIALIZER;
cola espera_este_oeste = {NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
cola espera_norte_sur = {NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
int n_este_oeste = 0;
int n_norte_sur = 0;
pthread_mutex_t mutex_este_oeste = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_norte_sur = PTHREAD_MUTEX_INITIALIZER;

FILE *log_file;
FILE *rand_file;
FILE *log_exit;

typedef struct {
    int espera_este_oeste;
    int espera_norte_sur;
    int crossing_vehicle_id; // ID del vehiculo en la interseccion en este momento (-1 si es ninguno)
    char crossing_dir[16]; // "norte-sur" o "este-oeste"
    pthread_mutex_t display_mutex; // Mutex para proteger acceso a variables compartidas
} DisplayState;

DisplayState display_state = {
    .espera_este_oeste = 0,
    .espera_norte_sur = 0,
    .crossing_vehicle_id = -1,
    .crossing_dir = "",
    .display_mutex = PTHREAD_MUTEX_INITIALIZER
};

bool is_valid_integer(const char *str) {
    // null or empty check
    if (str == NULL || *str == '\0') {
        return false;
    }

    while (*str) {
        if (!isdigit(*str)) {
            return false;
        }
        str++;
    }
    return true;
}

void *mostrar_en_pantalla(void *arg) {
    int *status = (int *)arg;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000;

    while (*status == 1) {
        pthread_mutex_lock(&display_state.display_mutex);

        // Clear and redraw the layout
        clear();
        mvprintw(0, 10, "==== trafico interseccion simulacion ====");
        mvprintw(2, 5, "este-oeste cola: %d", display_state.espera_este_oeste);
        mvprintw(3, 5, "norte-sur cola: %d", display_state.espera_norte_sur);

        if (display_state.crossing_vehicle_id != -1) {
            mvprintw(5, 5, "Vehicle %d is crossing (%s).",
                     display_state.crossing_vehicle_id, display_state.crossing_dir);
        } else {
            mvprintw(5, 5, "No vehicle currently crossing.");
        }

        refresh();
        pthread_mutex_unlock(&display_state.display_mutex);

        // Refresh the display every 500ms
        nanosleep(&ts, NULL);;
    }
    // exiting gracefully
    pthread_exit(0);
}

char *time_now_ns() {
    struct timespec ts;
    struct tm *local_time;
    char *timestamp = malloc(30 * sizeof(char)); // Allocate extra space for nanoseconds
    if (timestamp == NULL) {
        perror("Failed to allocate memory for timestamp");
        exit(EXIT_FAILURE);
    }

    // Get the current time with nanoseconds
    clock_gettime(CLOCK_REALTIME, &ts);

    // Convert seconds to local time
    local_time = localtime(&ts.tv_sec);

    // Format time as "YYYY-MM-DD HH:MM:SS.nnnnnnnnn"
    strftime(timestamp, 30, "%Y-%m-%d %H:%M:%S", local_time);
    sprintf(timestamp + 19, ".%09ld", ts.tv_nsec); // Append nanoseconds

    return timestamp; // Caller is responsible for freeing this memory
}

void *vehiculo_en_marcha(void *arg) {
    // usando la estructura vehiculo, se pasan los parametros para saber en que via y a cual direccion va
    vehiculo *coche_para_cola = (vehiculo *) malloc(sizeof(vehiculo));
    // se copian los datos del vehiculo para evitar corrupcion
    coche_para_cola->id = ((vehiculo *) arg)->id;
    coche_para_cola->derecha = ((vehiculo *) arg)->derecha;
    coche_para_cola->via = ((vehiculo *) arg)->via;

    // agregar a cola correspondiente

    if (coche_para_cola->via == este_oeste) {
        agregar(&espera_este_oeste, coche_para_cola);
        pthread_mutex_lock(&mutex_este_oeste);
        n_este_oeste++;
        if (debug) {
            printf("cola este-oeste: %d\n", n_este_oeste);
        }


        pthread_mutex_lock(&display_state.display_mutex);
        display_state.espera_este_oeste = n_este_oeste;
        pthread_mutex_unlock(&mutex_este_oeste);
        pthread_mutex_unlock(&display_state.display_mutex);
    } else if (coche_para_cola->via == norte_sur) {
        agregar(&espera_norte_sur, coche_para_cola);
        pthread_mutex_lock(&mutex_norte_sur);
        n_norte_sur++;

        pthread_mutex_lock(&display_state.display_mutex);
        if (debug) {
            printf("cola norte-sur: %d\n", n_norte_sur);
        }

        display_state.espera_norte_sur = n_norte_sur;
        pthread_mutex_unlock(&mutex_norte_sur);
        pthread_mutex_unlock(&display_state.display_mutex);
    }

    // esperar un poco para que se agreguen vehiculos a la lista de espera
    sleep(1);
    // sacar primero vehiculo de cola de espera de una de las dos vias
    // si ambas colas tienen vehiculos pendientes
    vehiculo *coche = NULL;
    bool coche_sacado = false; // flag para indicar que ya se saco un vehiculo de la lista de espera
    // impedir que se saquen mas que un vehiculo en un mismo hilo

    // asegurar que nadie pueda acceder a las colas mientras se leen parametros
    pthread_mutex_lock(&mutex_este_oeste);
    if (n_este_oeste > 0 && turno_este_oeste) {
        // a partir de aqui no se trabaja mas con espera_norte_sur, asi se puede soltar el mutex

        coche = sacar(&espera_este_oeste);
        n_este_oeste--;
        if (debug) {
            printf("cola este-oeste: %d\n", n_este_oeste);
        }

        pthread_mutex_lock(&display_state.display_mutex);
        display_state.espera_este_oeste = n_este_oeste;
        pthread_mutex_unlock(&display_state.display_mutex);

        if (!coche->derecha) {
            pthread_mutex_lock(&mutex_turno);
            turno_este_oeste = false;
            pthread_mutex_unlock(&mutex_turno);
        }
        coche_sacado = true;
    }
    pthread_mutex_unlock(&mutex_este_oeste);

    pthread_mutex_lock(&mutex_norte_sur);
    if (!coche_sacado && n_norte_sur > 0) {
        coche = sacar(&espera_norte_sur);
        n_norte_sur--;

        if (debug) {
            printf("cola norte-sur: %d\n", n_norte_sur);
        }

        pthread_mutex_lock(&display_state.display_mutex);
        display_state.espera_norte_sur = espera_norte_sur.tamano;
        pthread_mutex_unlock(&display_state.display_mutex);

        pthread_mutex_lock(&mutex_turno);
        turno_este_oeste = true;
        pthread_mutex_unlock(&mutex_turno);

        coche_sacado = true;
    }
    pthread_mutex_unlock(&mutex_norte_sur);

    pthread_mutex_lock(&mutex_este_oeste);
    if (!coche_sacado && n_este_oeste > 0) {
        // en el caso de que la lista de espera de norte-sur esta vacia y no es el turno de este-oeste,
        // entonces no pasa ningun vehiculo, para tratar esto aquí se da la posibilidad de que pueda pasar
        // un vehiculo de este-oeste

        coche = sacar(&espera_este_oeste);
        n_este_oeste--;
        if (debug) {
            printf("cola este-oeste: %d\n", n_este_oeste);
        }

        pthread_mutex_lock(&display_state.display_mutex);
        display_state.espera_este_oeste = espera_este_oeste.tamano;
        pthread_mutex_unlock(&display_state.display_mutex);

        if (!coche->derecha) {
            pthread_mutex_lock(&mutex_turno);
            turno_este_oeste = false;
            pthread_mutex_unlock(&mutex_turno);
        }
        coche_sacado = true;
    }
    pthread_mutex_unlock(&mutex_este_oeste);

    // el coche ahora nunca deberia ser un puntero a NULL, porque cuando hay un hilo, hay un vehiculo esperando
    // en alguna de las dos vias
    // supongamos que solamente hay un vehiculo en una de las dos vias, entonces se agrega a la cola de espera
    // y despues comprueba
    // 1) hay vehiculos en via este-oeste y es el turno de este-oeste -> vehiculo esta permitido pasar la interseccion
    // 2) hay vehiculos en via norte-sur -> primer vehiculo esta permitido pasar la interseccion
    // 3) en el caso de que no fue el turno de este-oeste y no hay vehiculos esperando en via norte-sur, se da el paso
    // a la via este-oeste para asegurar el progreso de vehiculos
    // si hay solamente un vehiculo en la via este-oeste, ahi se tiene que dar el paso a este vehiculo
    // concludiendo, siempre algun vehiculo esta elegido para pasar la interseccion

    if (coche != NULL) {
        if (coche->via == este_oeste && coche->derecha) {
            // en case de la via este-oeste que el vehicula gira a la derecha, no tiene que esperar al paso a la interseccion
            sleep(1); // tiempo para girar a derecha
            if (debug) {
                printf("girando a derecha -> id %d\n", coche->id);
            }
            pthread_mutex_lock(&display_state.display_mutex);
            display_state.crossing_vehicle_id = coche->id;
            strcpy(display_state.crossing_dir, "girando a la derecha hacia norte");
            pthread_mutex_unlock(&display_state.display_mutex);

            fprintf(log_file, "[%s] Vehiculo %d del %s gira al norte.\n",
                    time_now_ns(), coche->id, coche->via == este_oeste ? "este-oeste" : "norte-sur");
            free(coche);
            pthread_exit(0);
        }

        sem_wait(&sem);
        fprintf(log_file, "[%s] Vehiculo %d del %s esta pasando.\n",
                time_now_ns(), coche->id, coche->via == este_oeste ? "este-oeste" : "norte-sur");
        pthread_mutex_lock(&display_state.display_mutex);
        display_state.crossing_vehicle_id = coche->id;
        strcpy(display_state.crossing_dir, coche->via == este_oeste ? "este-oeste" : "norte-sur");
        pthread_mutex_unlock(&display_state.display_mutex);
        sleep(2); // tiempo para cruzar
        sem_post(&sem);

        fprintf(log_file, "[%s] Vehiculo %d del %s se fue de la interseccion.\n",
                time_now_ns(), coche->id, coche->via == este_oeste ? "este-oeste" : "norte-sur");

        free(coche);
        pthread_exit(0);
    }

    char *timestamp = time_now_ns();
    fprintf(log_file, "[%s] no se saco cocha en hilo de coche %d - %s\n", timestamp, coche_para_cola->id,
            coche_para_cola->via == este_oeste ? "este-oeste" : "norte-sur");
    free(timestamp);
    if (debug) {
        printf("-------- coche es nulo ----- se agrego: id %d via %s\n", coche_para_cola->id,
               coche_para_cola->via == este_oeste ? "este-oeste" : "norte-sur");
    }
    int *int_ptr = malloc(sizeof(int));
    *int_ptr = 1;
    pthread_exit((void *) int_ptr);
}

int main(int argc, char **argv) {
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = 100000000;
    // comprobar si cadena es entero: https://www.geeksforgeeks.org/check-given-string-valid-number-integer-floating-point/
    if (argc == 3 && !(!is_valid_integer(argv[1]) || argv[2] == "-v")) {
        printf("se usa output verbose para debugging\n");
        debug = true;
    } else if (argc != 2 || !is_valid_integer(argv[1])) {
        fprintf(stderr, "usage: ./semaforo <positive integer>, for example: './semaforo 123'\n");
        return -1;
    }

    const int RANGE_MIN = 0;
    const int RANGE_MAX = 5000;

    const int MODULO_VIA_NORTE_SUR = 5;
    const int MODULO_VIA_ESTE_OESTE = 4;
    const int MODULO_VIA_ESTE_NORTE = 9;

    log_file = fopen("logs/debug.log", "w");
    rand_file = fopen("logs/rand.log", "w");

    // Inicializar ncurses
    if (!debug) {
        initscr();
        cbreak();
        noecho();
        curs_set(0);
    }

    // inicializar semaforo para cruzar
    sem_init(&sem, 0, 1);

    // se usa atoi() aqui aunque no reporta errores de conversion, pero se sabe que el string es convertible porque eso
    // se controla antes con la funcion is_valid_integer()
    unsigned int seed = (unsigned int) atoi(argv[1]);
    if (debug) {
        printf("#### Inicializando Semaforo con seed=%u ####\n", seed);
    }

    int display_status = 1; // mientras que no este 0 el programa sigue ejecutandose
    pthread_t display_thread;
    pthread_create(&display_thread, NULL, mostrar_en_pantalla, &display_status);

    // 1) crear numero aleatorio
    int status = 1;
    int contador = 0;
    int next_id = 0;
    int contador_este_norte = 0;
    int contador_este_oeste = 0;
    int contador_norte_sur = 0;
    while (status == 1) {
        contador++;

        int rd_num = rand_r(&seed) % (RANGE_MAX - RANGE_MIN + 1) + RANGE_MIN;

        vehiculo *coche = NULL;
        // usar modulo para llegada aleatoria de vehiculos en ambas vias
        if (rd_num % MODULO_VIA_NORTE_SUR == 0) {
            coche = crear_vehiculo(next_id, norte_sur, true);
            next_id++;
            contador_norte_sur++;
        } else if (rd_num % MODULO_VIA_ESTE_OESTE == 0) {
            // vehiculo que continua derecho
            coche = crear_vehiculo(next_id, este_oeste, false);
            next_id++;
            contador_este_oeste++;
        } else if (rd_num % MODULO_VIA_ESTE_NORTE == 0) {
            // vehiculo que gira a la dereche
            coche = crear_vehiculo(next_id, este_oeste, true);
            next_id++;
            contador_este_norte++;
        }

        if (debug && coche != NULL) {
            // crear hilo pasando el coche correspondiente
            pthread_create(&hilos[next_id - 1], NULL, vehiculo_en_marcha, coche);
            vias type = coche->via;
            if (type == 0) {
                printf("    vehiculo en via norte-sur creado\n");
            } else if (type == 1) {
                if (!coche->derecha) {
                    printf("    vehiculo en via este-oeste creado (siuge derecho)\n");
                } else {
                    printf("    vehiculo en via este-oeste creado (gira a la derecha)\n");
                }
            }
        }

        if (contador > 25) {
            // simular solamente 100 ciclos primero
            status = 0;
        }
        nanosleep(&req, &rem);
        if (coche != NULL) {
            char *timestamp = time_now_ns();
            fprintf(rand_file, "[%s] id: %d | via: %s | dir: %s\n", timestamp, coche->id,
                    coche->via == este_oeste ? "este-oeste" : "norte-sur",
                    coche->derecha == 1 ? "derecha" : "recto");
            free(timestamp);
        }
        if (next_id == NUM_VEHICULOS) {
            // si se excede el numero de sitios en el arreglo, se termina la llegada de vehiculos
            status = 0;
        }
    }

    fprintf(rand_file, "num vehiculos total: %d", next_id);
    fclose(rand_file);

    log_exit = fopen("logs/thread_exit.log", "w");
    fprintf(log_exit, "Thread Exit Log\n");
    fclose(log_exit);

    // esperar la terminacion de todos los hilos creados

    char *timestamp = "";
    for (int i = 0; i < next_id; i++) {
        void *exit_status;
        // agregar a archivo log exisitendo, asi se guarda informacion de los demas hilos en
        // caso de que uno no termina
        log_exit = fopen("logs/thread_exit.log", "a");
        pthread_join(hilos[i], &exit_status);
        int st = exit_status ? *(int *) exit_status : -1;
        timestamp = time_now_ns();
        if (st == -1) {
            fprintf(log_exit, "[%s] Thread %d terminated with no status\n", timestamp, i);
        } else {
            fprintf(log_exit, "[%s] Thread %d terminated with status %d\n", timestamp, i, st);
        }
        fclose(log_exit);
        free(exit_status);
        free(timestamp);
    }


    fprintf(log_file, "Debug message\n");
    fprintf(log_file, "este-oeste: %d\n", contador_este_oeste);
    fprintf(log_file, "norte-sur: %d\n", contador_norte_sur);
    fprintf(log_file, "este-norte (gira): %d\n", contador_este_norte);

    fclose(log_file);

    sem_destroy(&sem);

    // esperar a hilo de pantalla para terminar

    log_exit = fopen("logs/thread_exit.log", "a");
    display_status = 0;
    pthread_join(display_thread, NULL);
    char* exit_time = time_now_ns();
    fprintf(log_exit, "[%s] Display thread terminated\n", exit_time);
    fclose(log_exit);
    free(exit_time);

    if (!debug) {
        endwin();
    }

    return 0;
}
