#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define BUF_SIZE 4096
#define NAME_LEN 128

typedef struct {
    size_t len;
    int last;
    int stop;
    char data[BUF_SIZE];
} shm_buf_t;

#endif