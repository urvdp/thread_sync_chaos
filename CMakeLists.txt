cmake_minimum_required(VERSION 3.16)
project(thread_sync_chaos C)

set(CMAKE_C_STANDARD 11)

# Add the executable target with source files
add_executable(${PROJECT_NAME}
        src/main.c
        src/vehiculo.c
        src/cola.c
        src/pantalla.c
        src/utils.c)

# Add include directories for this target
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE pthread ncurses)

# Optional: Debugging flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -Iinclude")