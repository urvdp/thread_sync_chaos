#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <pthread.h>
#include <semaphore.h>

#include "vehiculo.h"
#include "cola.h"
#include "pantalla.h"
#include "utils.h"

#define NUM_VEHICULOS 100

bool debug = false;

// parametros necesitados para asegurar sincronizacion entre procesos usando posix utilidadas incluyendo pthread y semaphore
sem_t sem;
pthread_t hilos[NUM_VEHICULOS];
bool turno_este_oeste = true; // flag para que las posibilidades en ambas colas de espera sean iguales
pthread_mutex_t mutex_turno = PTHREAD_MUTEX_INITIALIZER;
cola espera_este_oeste = {NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
cola espera_norte_sur = {NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
int n_este_oeste = 0; // para representar numero de vehiculos en la cola de la via este oeste
int n_norte_sur = 0; // para representar numero de vehiculos en la cola de la via norte sur
pthread_mutex_t mutex_este_oeste = PTHREAD_MUTEX_INITIALIZER; // proteger acceso n_este_oeste
pthread_mutex_t mutex_norte_sur = PTHREAD_MUTEX_INITIALIZER; // proteger acceso n_norte_sur

// semaforo para que solamente el primero en la fila de la via oeste-este cruza
// (evitar que los que giran a la derecha adelanten)
pthread_mutex_t mutex_primero_o_e = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_n_s_block = PTHREAD_MUTEX_INITIALIZER;

// definicion de punteros a archivos log
FILE *log_file;
FILE *rand_file;
FILE *log_exit;

void conducir_desde_norte(vehiculo *coche) {
    // funcion si el vehiculo de la fila este-oeste fue eligido para cruzar

    // solamente el primero de la fila puede postular sem_wait, cuando recibe paso el proximo
    // es el primero y tiene derecho a postular sem_wait()
    // sin esta logica, los de la via este-oeste tienen que esperar a ser los primeros en la
    // fila de este-oeste hasta que pueden solicitar el pase por la cruce, mientras que los
    // en la via norte-sur solicitan el pase sin ser el primero sin restriccion
    // asi los de la via este-oeste no se les deja entrar porque solicitan sem_wait
    // mucho mas tarde que los del norte-sur
    pthread_mutex_lock(&mutex_n_s_block);
    sem_wait(&sem);
    pthread_mutex_unlock(&mutex_n_s_block);

    char *timecheck = time_now_ns();
    fprintf(log_file, "[%s] vehiculo %d del %s esta pasando.\n",
            timecheck, coche->id, coche->via == este_oeste ? "este-oeste" : "norte-sur");
    free(timecheck);
    pthread_mutex_lock(&display_state.display_mutex);
    display_state.crossing_vehicle_id = coche->id;
    strcpy(display_state.crossing_dir, coche->via == este_oeste ? "este-oeste" : "norte-sur");
    display_state.espera_norte_sur--;
    display_state.pasados_n_s++;

    pthread_mutex_unlock(&display_state.display_mutex);
    sleep(3); // tiempo para cruzar

    sem_post(&sem);
}

void conducir_desde_este(vehiculo *coche) {
    // funcion si el vehiculo de la fila norte-sur fue eligido para cruzar

    // mantener orden que solamente el primero en via este-oeste pueda cruzar
    pthread_mutex_lock(&mutex_primero_o_e);
    char *timestamp = time_now_ns();
    fprintf(log_file, "[%s] vehiculo %d es primero en via este-oeste (%s)\n", timestamp, coche->id,
            coche->derecha ? "girando" : "derecha");
    free(timestamp);

    if (coche->derecha) {
        // en case de la via este-oeste que el vehicula gira a la derecha, no tiene que esperar al paso a la interseccion
        sleep(1); // tiempo para girar a derecha
        char *timecheck = time_now_ns();
        fprintf(log_file, "[%s] vehiculo %d del %s gira al norte.\n",
                timecheck, coche->id, coche->via == este_oeste ? "este-oeste" : "norte-sur");
        free(timecheck);
        if (debug) {
            printf("girando a derecha -> id %d\n", coche->id);
        }
        // vehiculo se fue de la cruce, proximo en via este oeste puede cruzar
        pthread_mutex_unlock(&mutex_primero_o_e);
        char *timestamp2 = time_now_ns();
        fprintf(log_file, "[%s] vehiculo %d se fue de la cruce (%s)\n", timestamp2, coche->id,
                coche->derecha ? "girando" : "derecho");
        free(timestamp2);

        pthread_mutex_lock(&display_state.display_mutex);
        display_state.crossing_vehicle_id = coche->id;
        strcpy(display_state.crossing_dir, "girando a la derecha");
        display_state.espera_este_oeste--;
        display_state.pasados_e_n++;
        pthread_mutex_unlock(&display_state.display_mutex);
    } else {
        sem_wait(&sem);
        char *timecheck = time_now_ns();
        fprintf(log_file, "[%s] vehiculo %d del %s esta pasando.\n",
                timecheck, coche->id, coche->via == este_oeste ? "este-oeste" : "norte-sur");
        free(timecheck);

        pthread_mutex_lock(&display_state.display_mutex);
        display_state.crossing_vehicle_id = coche->id;
        strcpy(display_state.crossing_dir, coche->via == este_oeste ? "este-oeste" : "norte-sur");
        display_state.espera_este_oeste--;
        display_state.pasados_e_o++;
        pthread_mutex_unlock(&display_state.display_mutex);

        sleep(3); // tiempo para cruzar
        // vehiculo se fue de la cruce, senyalar al pimer vehiculo en la fila este-oeste
        // que puede cruzar
        pthread_mutex_unlock(&mutex_primero_o_e);
        char *timestamp = time_now_ns();
        fprintf(log_file, "[%s] vehiculo %d se fue de la cruce (%s).\n", timestamp, coche->id,
                coche->derecha ? "girando" : "derecha");
        free(timestamp);
        sem_post(&sem);
    }
}


// funcion principal de hilos
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
        pthread_mutex_unlock(&mutex_este_oeste);

        pthread_mutex_lock(&display_state.display_mutex);
        display_state.espera_este_oeste++;
        pthread_mutex_unlock(&display_state.display_mutex);
    } else if (coche_para_cola->via == norte_sur) {
        agregar(&espera_norte_sur, coche_para_cola);
        pthread_mutex_lock(&mutex_norte_sur);
        n_norte_sur++;
        if (debug) {
            printf("cola norte-sur: %d\n", n_norte_sur);
        }
        pthread_mutex_unlock(&mutex_norte_sur);

        pthread_mutex_lock(&display_state.display_mutex);
        display_state.espera_norte_sur++;
        pthread_mutex_unlock(&display_state.display_mutex);
    }

    // esperar un poco para que se agreguen vehiculos a la lista de espera
    sleep(1);
    free(arg); // liberar memoria solamente aqui para que se liberara ya en el main while
    int *int_ptr = malloc(sizeof(int));
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
        // el contador n_este_oeste / n_norte_sur es importante para saber cuantos aun
        // quieren pasar de los agregados a la cola a un hilo para pasar la interseccion, pero
        // los contadores no son sincronos con los vehiculos pasando por la interseccion
        // explicacion comportamiento observado:
        // lo que esta pasando es que se asignan los vehiculos de la lista de espera a un hilo
        // y justo en este momento se van de la lista de espera, pero realmente en este estado activo
        // que hayan sido elegido de un hilo para pasar la interseccion todavia esperan a su paso
        // por el semaforo, osea de las dos colas para ambos vias, los vehiculos son asignados a un hilo
        // y pasan a ser activo en la espera al paso
        // de hecho las dos colas de ambas vias se juntan a una cola activa (con postulando sem_wait) para el paso
        // que en este momento sin esa cola activa no esta representado por n_este_oeste / n_nort_sur resultando
        // en ese comportamiento asincrono
        // cuando se llama sem_wait hasta que se yo, no necesariamente es una cola FIFO
        // que el primero que llamo sem_wait es el proximo en recibir el paso cuando el semaforo esta
        // disponible otra vez, eso depende de la implementacion del OS, pero de las pruebas que hice
        // yo me parece que en esta implementacion el primero en llamar es el proximo en recibir el paso

        // mas encima, los vehiculos que van a la derecha no tienen que esperar su paso por la interseccion
        // porque no se tienen que sincronizar con la via norte-sur como son independientes
        // pero lo que si tienen que esperar es que tienen que ser los primeros en la fila de la via este-oeste
        // para poder cruzar -> para esto otro semaforo es necesitado para que solamente el primero
        // en la via este-oeste cruza y los que van a la derecha no adelanten los que esperan a pasar derecho
        // la cuestion es que los vehiculos que pasan al oeste tienen que esperar por el paso de solamente uno
        // asi automaticamente no adelantan

        if (coche->via == este_oeste) {
            conducir_desde_este(coche);
        } else {
            conducir_desde_norte(coche);
        }

        char *timestamp = time_now_ns();
        fprintf(log_file, "[%s] vehiculo %d del %s se fue de la cruce.\n",
                timestamp, coche->id, coche->via == este_oeste ? "este-oeste" : "norte-sur");
        free(timestamp);
        free(coche);
        *int_ptr = 0;
        pthread_exit((void *) int_ptr);
    }

    char *timestamp = time_now_ns();
    fprintf(log_file, "[%s] no se saco cocha en hilo de coche %d - %s\n", timestamp, coche_para_cola->id,
            coche_para_cola->via == este_oeste ? "este-oeste" : "norte-sur");
    free(timestamp);
    if (debug) {
        printf("-------- coche es nulo ----- se agrego: id %d via %s\n", coche_para_cola->id,
               coche_para_cola->via == este_oeste ? "este-oeste" : "norte-sur");
    }

    *int_ptr = 1;
    pthread_exit((void *) int_ptr);
}

int main(int argc, char **argv) {
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = 200000000;
    // comprobar si cadena es entero:
    // https://www.geeksforgeeks.org/check-given-string-valid-number-integer-floating-point/
    if (argc == 3 && !(!is_valid_integer(argv[1]) || (!strcmp(argv[2], "-v") == 0))) {
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
    int user_exit_state;
    if (!debug) {
        init_pantalla(NUM_VEHICULOS, &user_exit_state);
    } else {
        user_exit_state = 0;
    }

    // inicializar semaforo para cruzar
    sem_init(&sem, 0, 1);

    // se usa atoi() aqui aunque no reporta errores de conversion, pero se sabe que el string es convertible porque eso
    // se controla antes con la funcion is_valid_integer()
    unsigned int seed = (unsigned int) atoi(argv[1]);
    pthread_t display_thread;
    int display_status = 1; // mientras que no este 0 el programa sigue ejecutandose
    if (debug) {
        printf("#### Inicializando Semaforo con seed=%u ####\n", seed);
    } else {
        pthread_create(&display_thread, NULL, mostrar_en_pantalla, &display_status);
    }

    // 1) crear numero aleatorio
    int status = 1;
    int contador = 0;
    int next_id = 0;
    int contador_este_norte = 0;
    int contador_este_oeste = 0;
    int contador_norte_sur = 0;
    time_t time_init = time(NULL);
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

        if (coche != NULL) {
            if (debug) {
                printf("Se creo un vehiculo %d\n", coche->id);
            }
            // crear hilo pasando el coche correspondiente
            pthread_create(&hilos[next_id - 1], NULL, vehiculo_en_marcha, coche);
            if (debug) {
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
            // actualizar contador de vehiculos llegados al cruce desde inicializaicon
            pthread_mutex_lock(&display_state.display_mutex);
            display_state.vehicles_arrived++;
            pthread_mutex_unlock(&display_state.display_mutex);
        }
        nanosleep(&req, &rem);
        if (coche != NULL) {
            char *timestamp = time_now_ns();
            fprintf(rand_file, "[%s] id: %d | via: %s | dir: %s\n", timestamp, coche->id,
                    coche->via == este_oeste ? "este-oeste" : "norte-sur",
                    coche->derecha == 1 ? "derecha" : "recto");
            free(timestamp);
            // Liberar memoria original despues de la creacion del hilo
            // el hilo maneja su propia copia de la memoria
            coche = NULL;
        }
        if (next_id == NUM_VEHICULOS || user_exit_state == 1) {
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
    time_t time_sim_end = time(NULL);

    fprintf(log_file, "Debug message\n");
    fprintf(log_file, "este-oeste: %d\n", contador_este_oeste);
    fprintf(log_file, "norte-sur: %d\n", contador_norte_sur);
    fprintf(log_file, "este-norte (gira): %d\n", contador_este_norte);

    fclose(log_file);

    sem_destroy(&sem);
    sleep(2);

    if (!debug) {
        // esperar a hilo de pantalla para terminar
        log_exit = fopen("logs/thread_exit.log", "a");
        display_status = 0;
        pthread_join(display_thread, NULL);
        char *exit_time = time_now_ns();
        fprintf(log_exit, "[%s] Display thread terminated\n", exit_time);
        fclose(log_exit);
        free(exit_time);
        reset_pantalla();
    }

    // dar unas estadisticas al usuario
    long time_delta_s = (long) (time_sim_end - time_init);
    long time_delta_min = time_delta_s / 60;
    time_delta_s = time_delta_s % 60;
    printf("==== Estadistica de simulacion con %d vehiculos ====\n", NUM_VEHICULOS);
    printf("     vehiculos  norte-sur total: %d\n", contador_norte_sur);
    printf("     vehiculos este-oeste total: %d\n", contador_este_norte + contador_este_oeste);
    printf("                           ->  derecho: %d \n", contador_este_oeste);
    printf("                           ->  girando: %d\n", contador_este_norte);
    printf("     -------------------------------------\n");
    printf("     vehiculos            total: %d\n", contador_este_norte + contador_este_oeste + contador_norte_sur);
    if (time_delta_s < 10) {
        printf("     tiempo total de simulacion: %ld:%d%ld min\n", time_delta_min, 0, time_delta_s);
    } else {
        printf("     tiempo total de simulacion: %ld:%ld min\n", time_delta_min, time_delta_s);
    }
    return 0;
}
