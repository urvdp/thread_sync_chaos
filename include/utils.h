//
// Created by jan on 19.11.24.
//

#ifndef UTILS_H
#define UTILS_H

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define TIMESTAMP_SIZE 30

char *time_now_ns();

bool is_valid_integer(const char *str);

#endif //UTILS_H
