//
// Created by jan on 18.11.24.
//

#include <pthread.h>
#include <unistd.h>

#include "vehiculo.h"

#ifndef COLA_H
#define COLA_H

typedef struct nodo {
    vehiculo* coche;
    struct nodo *prox;
} nodo;

typedef struct cola {
    nodo *frente, *fondo;
    int tamano;
    pthread_mutex_t candado;
    pthread_cond_t condicion;
} cola;

void agregar(cola *ref_cola, vehiculo* coche);

vehiculo* sacar(cola *ref_cola);

void imprimir_cola(cola* ref_cola);

#endif //COLA_H
