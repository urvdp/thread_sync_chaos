//
// Desplegue en pantalla usando ncurses
//

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#ifndef PANTALLA_H
#define PANTALLA_H

#define BUFFER_SIZE 3

#define COL_RED 1
#define COL_BLUE 2
#define COL_CYAN 3

#define USER_EXIT 'q'

#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "utils.h"

typedef struct {
    int espera_este_oeste;
    int espera_norte_sur;
    int crossing_vehicle_id; // ID del vehiculo en la interseccion en este momento (-1 si es ninguno)
    char crossing_dir[50]; // "norte-sur" o "este-oeste" o "girando a la derecha"
    pthread_mutex_t display_mutex; // mutex para proteger acceso a variables compartidas
    int vehicles_arrived; // muestra el estado temporal de los vehiculos que llegaron al cruce desde inicializacion
} DisplayState;



extern FILE *display_file;
extern DisplayState display_state;
extern int n_vehicles;


void init_pantalla(const int n_vehiculos, int* user_exit_state);

void reset_pantalla(void);

void *mostrar_en_pantalla(void *arg);


#endif //PANTALLA_H
