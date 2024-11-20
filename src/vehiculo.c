//
// Created by jan on 18.11.24.
//

#include "vehiculo.h"

#include <stdlib.h>

vehiculo *crear_vehiculo(int id, enum vias type, bool derecha) {
    vehiculo* new = (vehiculo*) malloc(sizeof(vehiculo));
    new->via = type;
    new->id = id;
    if (new->via == norte_sur) {
        // unica posibilidad para vehiculos de norte-sur
        new->derecha = true;
    } else {
        new->derecha = derecha;
    }
    return new;
}
