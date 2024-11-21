//
// Created by jan on 19.11.24.
//

#include "pantalla.h"

FILE *display_file = NULL;

DisplayState display_state = {
    .espera_este_oeste = 0,
    .espera_norte_sur = 0,
    .crossing_vehicle_id = -1,
    .crossing_dir = "",
    .display_mutex = PTHREAD_MUTEX_INITIALIZER
};

void init_pantalla(void) {
    initscr();

    if (!has_colors()) {
        reset_pantalla();
        fprintf(stderr, "Error: Your terminal does not support colors.\n");
        exit(EXIT_FAILURE); // Terminate the program safely
    }

    start_color(); // include color support, aragorn supports colors (I tried it)
    cbreak();
    noecho();
    curs_set(0);

    // init some colors for cars
    init_pair(COL_RED, COLOR_RED, COLOR_BLACK); // este-oeste derecho
    init_pair(COL_BLUE, COLOR_BLUE, COLOR_BLACK); // norte-sur
    init_pair(COL_CYAN, COLOR_CYAN, COLOR_BLACK); // este-oeste derecha
}

void reset_pantalla(void) {
    endwin();
}

void *mostrar_en_pantalla(void *arg) {
    int *status = (int *) arg;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000;

    int offset = 28;

    display_file = fopen("logs/display.log", "w");
    fprintf(display_file, "Display Log\n");

    // get max terminal height and width
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    fprintf(display_file, "configuracion terminal - rows: %d cols: %d\n", rows, cols);

    int buffer_este_oeste[BUFFER_SIZE];
    int buffer_norte_sur[BUFFER_SIZE];
    int buffer_girando[BUFFER_SIZE];

    for (int k = 0; k < BUFFER_SIZE; k++) {
        buffer_este_oeste[k] = -1;
        buffer_norte_sur[k] = -1;
        buffer_girando[k] = -1;
    }

    const int max_proc = 7;
    bool reset = true;

    int local_espera_este_oeste, local_espera_norte_sur = 0;
    int local_crossing_vehicle_id = -1;
    char local_crossing_dir[25];
    while (*status == 1) {
        // controlar si hay nuevo vehiculo en la interseccion
        if (local_crossing_vehicle_id != -1 && display_state.crossing_vehicle_id != local_crossing_vehicle_id) {
            reset = true;
        }

        pthread_mutex_lock(&display_state.display_mutex);
        local_espera_este_oeste = display_state.espera_este_oeste;
        local_espera_norte_sur = display_state.espera_norte_sur;
        local_crossing_vehicle_id = display_state.crossing_vehicle_id;
        strcpy(local_crossing_dir, display_state.crossing_dir);
        pthread_mutex_unlock(&display_state.display_mutex);

        // Clear and redraw the layout
        clear();
        attrset(A_NORMAL); // usar texto en default
        attron(A_BOLD);
        mvprintw(0, 5, "==== simulacion de interseccion de trafico ====");
        attroff(A_BOLD);
        attron(COLOR_PAIR(COL_RED));
        mvprintw(2, 5, "este-oeste cola: %d", local_espera_este_oeste);
        attron(COLOR_PAIR(COL_BLUE));
        mvprintw(4, 5, "norte-sur cola: %d", local_espera_norte_sur);
        attroff(COLOR_PAIR(COL_BLUE));

        int row_offset = 2;
        if (local_crossing_vehicle_id != -1) {
            mvprintw(row_offset + 9, offset, "Vehicle %d is crossing (%s).",
                     local_crossing_vehicle_id, local_crossing_dir);
        } else {
            mvprintw(row_offset + 9, offset, "No vehicle currently crossing.");
        }

        // display intersection

        // static section
        mvprintw(row_offset - 1, offset + 12, "norte");
        mvprintw(row_offset + 7, offset + 21, "sur");
        mvprintw(row_offset + 7, offset, "oeste");
        mvprintw(row_offset + 7, offset + 41, "este");
        for (int i = row_offset; i < (row_offset + 4); i++) {
            for (int j = 0; j < 18; j += 2) {
                mvprintw(i, offset + j, "#");
            }
            // leave 5 spaces
            if (i % 2 != 0) {
                mvprintw(i, offset + 18 + 4, "|");
            }
            for (int j = 0; j < 18; j += 2) {
                mvprintw(i, offset + 18 + 10 + j, "#");
            }
        }

        for (int j = 0; j < 46; j += 2) {
            mvprintw(row_offset + 6, offset + j, "#");
        }

        // dynamic section
        const int vehicle_spacing = 2;
        const int step_size = 3;

        // vehiculos en esperando en via este-oeste
        int east_west_col = 28;
        int east_west_row = row_offset + 5;

        int n_este_oeste = local_espera_este_oeste;
        if (n_este_oeste + offset + east_west_col > cols) {
            n_este_oeste = cols - offset - east_west_col;
        }
        attron(COLOR_PAIR(COL_RED));
        for (int k = 0; k < n_este_oeste; k++) {
            mvprintw(east_west_row, offset + east_west_col + (k * vehicle_spacing), "o");
        }

        // vehiculos esperando en via norte-sur
        int north_south_col = 19;
        int north_south_row = row_offset + 4;

        int n_norte_sur = local_espera_norte_sur;
        if (n_norte_sur > 6) {
            n_norte_sur = 6;
        }
        attron(COLOR_PAIR(COL_BLUE));
        for (int k = n_norte_sur; k > 0; k--) {
            mvprintw(north_south_row - k, offset + north_south_col, "o");
        }
        attroff(COLOR_PAIR(COL_BLUE));
        // procesamiento de cruce
        if (reset && local_crossing_vehicle_id != -1) {
            // hay un vehiculo cruzando la interseccion, trigger la ejecucion de mostrarlo en pantalla
            // este-oeste o norte-sur -> 2s, girar al norte -> 1s
            // tiempo de actualizacion: 0.4s
            // -1 significa que buffer esta vacio y se puede asignar vehiculo
            if (strcmp("este-oeste", local_crossing_dir) == 0) {
                // vehiculo pasa del este al oeste
                for (int k = 0; k < BUFFER_SIZE; k++) {
                    if (buffer_este_oeste[k] == -1) {
                        buffer_este_oeste[k] = 0;
                        break;
                    }
                }
            } else if (strcmp("norte-sur", local_crossing_dir) == 0) {
                // vehiculo pasa del norte al oeste
                for (int k = 0; k < BUFFER_SIZE; k++) {
                    if (buffer_norte_sur[k] == -1) {
                        buffer_norte_sur[k] = 0;
                        char *timestamp = time_now_ns();
                        fprintf(display_file, "[%s] added n/s at k %d\n", timestamp, k);
                        free(timestamp);
                        break;
                    }
                }
            } else if (strcmp("girando a la derecha", local_crossing_dir) == 0) {
                // vehiculo pasa del este al norte
                for (int k = 0; k < BUFFER_SIZE; k++) {
                    if (buffer_girando[k] == -1) {
                        buffer_girando[k] = 0;
                        break;
                    }
                }
            }
            reset = false;
        }


        // comprobar si hay vehiculos en el buffer cruzando y procesar
        for (int k = 0; k < BUFFER_SIZE; k++) {
            int estado_este_oeste = buffer_este_oeste[k];
            if (estado_este_oeste != -1) {
                attron(COLOR_PAIR(COL_RED));
                mvprintw(east_west_row, offset + east_west_col - step_size - step_size * estado_este_oeste, "o");
                buffer_este_oeste[k]++;
                if (estado_este_oeste == max_proc) {
                    // reset buffer
                    buffer_este_oeste[k] = -1;
                }
            }
            int estado_norte_sur = buffer_norte_sur[k];
            if (estado_norte_sur != -1) {
                char *timestamp = time_now_ns();
                fprintf(display_file, "[%s] n/s-buffer k %d | status: %d\n", timestamp, k, estado_norte_sur);
                free(timestamp);
                // manejando al oeste
                attron(COLOR_PAIR(COL_BLUE));
                mvprintw(north_south_row + 1, offset + north_south_col - step_size * estado_norte_sur, "o");
                buffer_norte_sur[k]++;
                if (estado_norte_sur == max_proc - 1) {
                    buffer_norte_sur[k] = -1;
                }
            }
            int estado_gira = buffer_girando[k];
            if (estado_gira != -1) {
                // haciendo la gira a la derecha al norte
                attron(COLOR_PAIR(COL_CYAN));
                if (estado_gira == 0) {
                    mvprintw(east_west_row, offset + east_west_col - 3, "o");
                    buffer_girando[k]++;
                } else {
                    mvprintw(east_west_row - estado_gira, offset + east_west_col - 3, "o");
                    buffer_girando[k]++;
                    if (estado_gira == max_proc) {
                        buffer_girando[k] = -1;
                    }
                }
            }
        }

        char *timestamp = time_now_ns();
        fprintf(display_file, "[%s] e/o: %d | n/s: %d\n", timestamp, local_espera_este_oeste, local_espera_norte_sur);
        free(timestamp);
        refresh();
        nanosleep(&ts, NULL);
    }

    fclose(display_file);
    // exiting gracefully
    pthread_exit(0);
}
