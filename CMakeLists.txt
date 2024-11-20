cmake_minimum_required(VERSION 3.29)
project(sistemas_operativos_tarea3 C)

set(CMAKE_C_STANDARD 11)

add_executable(sistemas_operativos_tarea3 src/main.c
        src/vehiculo.c
        include/vehiculo.h
        src/cola.c
        include/cola.h
        src/pantalla.c
        include/pantalla.h
        src/utils.c
        include/utils.h)
