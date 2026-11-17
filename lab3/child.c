#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"

static int is_vowel(unsigned char c)
{
    c = (unsigned char)tolower(c);
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

static size_t remove_vowels(const char *src, size_t len, char *dst)
{
    size_t i;
    size_t o = 0;

    for (i = 0; i < len; i++) {
        if (!is_vowel((unsigned char)src[i])) {
            dst[o++] = src[i];
        }
    }
    return o;
}

static int sem_wait_retry(sem_t *s)
{
    while (sem_wait(s) == -1) {
        if (errno != EINTR) {
            perror("sem_wait");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int fd;
    shm_buf_t *buf;
    sem_t *empty;
    sem_t *full;
    FILE *out;
    char *tmp;
    int status = EXIT_SUCCESS;

    if (argc != 5) {
        fprintf(stderr, "Использование: %s shm empty full file\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = shm_open(argv[1], O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    buf = mmap(NULL, sizeof(shm_buf_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    empty = sem_open(argv[2], 0);
    if (empty == SEM_FAILED) {
        perror("sem_open empty");
        munmap(buf, sizeof(shm_buf_t));
        return EXIT_FAILURE;
    }

    full = sem_open(argv[3], 0);
    if (full == SEM_FAILED) {
        perror("sem_open full");
        sem_close(empty);
        munmap(buf, sizeof(shm_buf_t));
        return EXIT_FAILURE;
    }

    out = fopen(argv[4], "w");
    if (out == NULL) {
        perror("fopen");
        sem_close(empty);
        sem_close(full);
        munmap(buf, sizeof(shm_buf_t));
        return EXIT_FAILURE;
    }

    tmp = malloc(BUF_SIZE);
    if (tmp == NULL) {
        perror("malloc");
        fclose(out);
        sem_close(empty);
        sem_close(full);
        munmap(buf, sizeof(shm_buf_t));
        return EXIT_FAILURE;
    }

    while(1) {
        size_t n;

        if (sem_wait_retry(full) == -1) {
            status = EXIT_FAILURE;
            break;
        }

        if (buf->stop) {
            sem_post(empty);
            break;
        }

        n = remove_vowels(buf->data, buf->len, tmp);

        if (n > 0) {
            if (fwrite(tmp, 1, n, out) != n) {
                perror("fwrite");
                status = EXIT_FAILURE;
            }
        }
        if (buf->last) {
            fputc('\n', out);
            fflush(out);
        }

        if (sem_post(empty) == -1) {
            perror("sem_post");
            status = EXIT_FAILURE;
            break;
        }
    }

    free(tmp);
    if (fclose(out) != 0) {
        perror("fclose");
        status = EXIT_FAILURE;
    }
    sem_close(empty);
    sem_close(full);
    munmap(buf, sizeof(shm_buf_t));
    return status;
}