//
// Created by jan on 18.11.24.
//

#include "cola.h"

#include <stdlib.h>

void agregar(cola *ref_cola, vehiculo *coche) {
    if (coche->id == 7) {
        printf("agregar: before mem allocation\n");
    }
    nodo *new = (nodo *) malloc(sizeof(nodo));
    if (new == NULL) {
        fprintf(stderr, "agregar a cola: allocacion de memoria fallo.\n");
        return;
    }
    new->coche = coche;
    new->prox = NULL;

    if (new->coche->id == 7) {
        printf("agregar: after mem allocation\n");
    }

    int lock_status = pthread_mutex_lock(&ref_cola->candado);
    if (coche->id == 7) {
        printf("agregar: lock status %d\n", lock_status);
    }

    if (lock_status != 0) {
        fprintf(stderr, "Thread %d: Failed to acquire mutex (Error: %d)\n", coche->id, lock_status);
        return;
    }
    if (new->coche->id == 7) {
        printf("agregar: candado obtenido -> no recibe mutex\n");
    }
    sleep(2);
    if (ref_cola->frente == NULL) {
        ref_cola->frente = ref_cola->fondo = new;
    } else {
        ref_cola->fondo->prox = new;
        ref_cola->fondo = new;
    }

    if (new->coche->id == 7) {
        printf("agregar: id7 esta en cola\n");
    }

    ref_cola->tamano++;

    printf("Queued vehiculo: %p, ID: %d VIA: %s\n", (void *) coche, coche->id,
           coche->via == este_oeste ? "este-oeste" : "norte-sur");

    // signalizar que hay un nuevo vehiculo
    //pthread_cond_signal(&ref_cola->condicion);
    // soltar candado
    pthread_mutex_unlock(&ref_cola->candado);
}

vehiculo *sacar(cola *ref_cola) {
    // cuando se llama antes el mutex ->candado tiene que haber sido aplicado
    pthread_mutex_lock(&ref_cola->candado);

    // si la cola esta vacia, se espera a un vehiculo -> en caso de sincronizacion de procesos real
    // creo que seria una muy buena implementacion, pero eso lo hace dificil desplegar lo que pasa
    // por eso trabajo mas con el tamano de las listas de espera para poder mostrarlo en pantalla
    //while (ref_cola->frente == NULL) {
    //    // mientras que se espera se suelta el candado para que se puedan agregar elementos
    //    pthread_cond_wait(&ref_cola->condicion, &ref_cola->candado);
    //}

    nodo *temp = ref_cola->frente;
    vehiculo *coche = temp->coche;
    ref_cola->frente = ref_cola->frente->prox;

    if (ref_cola->frente == NULL) {
        ref_cola->fondo = NULL; // cola esta vacia ahora
    }

    free(temp);
    ref_cola->tamano--;

    printf("Dequeued vehiculo: %p, ID: %d VIA: %s\n", (void *) coche, coche->id,
           coche->via == este_oeste ? "este-oeste" : "norte-sur");
    pthread_mutex_unlock(&ref_cola->candado);
    return coche;
}

void imprimir_cola(cola *ref_cola) {
    pthread_mutex_lock(&ref_cola->candado);
    if (ref_cola->tamano == 0) {
        printf("cola vacia");
    } else {
        nodo *actual = ref_cola->frente;
        pthread_mutex_lock(&ref_cola->candado);
        while (actual != NULL) {
            printf("Traversing: ID=%d Address=%p ", actual->coche->id, (void *) actual);
            printf("%d - ", actual->coche->id);
            actual = actual->prox;
        }
        pthread_mutex_unlock(&ref_cola->candado);
        printf("\n");
    }
    pthread_mutex_unlock(&ref_cola->candado);
}
