//
// Created by jan on 19.11.24.
//

#include "utils.h"

char *time_now_ns() {
    struct timespec ts;
    struct tm local_time;
    char *timestamp = malloc(TIMESTAMP_SIZE);
    if (timestamp == NULL) {
        perror("Failed to allocate memory for timestamp");
        return NULL;
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("Failed to get current time");
        free(timestamp);
        return NULL;
    }

    if (localtime_r(&ts.tv_sec, &local_time) == NULL) {
        perror("Failed to convert time to local time");
        free(timestamp);
        return NULL;
    }

    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", &local_time);
    size_t offset = strlen(timestamp);
    sprintf(timestamp + offset, ".%09ld", ts.tv_nsec);

    return timestamp; // usuario es responsable para liberar memoria con un free()
}

bool is_valid_integer(const char *str) {
    // null or empty check
    if (str == NULL || *str == '\0') {
        return false;
    }

    while (*str) {
        if (!isdigit(*str)) {
            return false;
        }
        str++;
    }
    return true;
}