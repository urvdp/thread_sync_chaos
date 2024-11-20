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
    cbreak();
    noecho();
    curs_set(0);
}

void reset_pantalla(void) {
    endwin();
}

void *mostrar_en_pantalla(void *arg) {
    int *status = (int *)arg;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000;

    display_file = fopen("logs/display.log", "w");
    fprintf(display_file, "Display Log\n");

    while (*status == 1) {
        int local_espera_este_oeste, local_espera_norte_sur, local_crossing_vehicle_id;
        char local_crossing_dir[25];

        pthread_mutex_lock(&display_state.display_mutex);
        local_espera_este_oeste = display_state.espera_este_oeste;
        local_espera_norte_sur = display_state.espera_norte_sur;
        local_crossing_vehicle_id = display_state.crossing_vehicle_id;
        strcpy(local_crossing_dir, display_state.crossing_dir);
        pthread_mutex_unlock(&display_state.display_mutex);

        // Clear and redraw the layout
        clear();
        mvprintw(0, 10, "==== trafico interseccion simulacion ====");
        mvprintw(2, 5, "este-oeste cola: %d", local_espera_este_oeste);
        mvprintw(3, 5, "norte-sur cola: %d", local_espera_norte_sur);

        if (local_crossing_vehicle_id != -1) {
            mvprintw(5, 5, "Vehicle %d is crossing (%s).",
                     local_crossing_vehicle_id, local_crossing_dir);
        } else {
            mvprintw(5, 5, "No vehicle currently crossing.");
        }

        char* timestamp = time_now_ns();
        fprintf(display_file, "[%s] e/o: %d | n/s: %d\n", timestamp, local_espera_este_oeste, local_espera_norte_sur);
        free(timestamp);
        refresh();
        nanosleep(&ts, NULL);
    }

    fclose(display_file);
    // exiting gracefully
    pthread_exit(0);
}