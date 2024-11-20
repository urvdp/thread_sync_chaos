//
// Created by jan on 18.11.24.
//

#include <stdbool.h>
#include <stdio.h>

#ifndef VEHICULO_H
#define VEHICULO_H

typedef enum vias {norte_sur, este_oeste} vias;

typedef struct vehiculo {
    // podria agregar tiempo llegada
    int id;
    enum vias via;
    bool derecha;
} vehiculo;

vehiculo *crear_vehiculo(int id, enum vias type, bool derecha);

#endif //VEHICULO_H
