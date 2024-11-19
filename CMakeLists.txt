cmake_minimum_required(VERSION 3.29)
project(sistemas_operativos_tarea3 C)

set(CMAKE_C_STANDARD 11)

add_executable(sistemas_operativos_tarea3 main.c
        vehiculo.c
        vehiculo.h
        cola.c
        cola.h)
