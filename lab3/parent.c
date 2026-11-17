#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"

typedef struct {
    char shm_name[NAME_LEN];
    char empty_name[NAME_LEN];
    char full_name[NAME_LEN];
    int fd;
    shm_buf_t *buf;
    sem_t *empty;
    sem_t *full;
    pid_t pid;
} channel_t;

static void channel_init(channel_t *c)
{
    c->fd = -1;
    c->buf = MAP_FAILED;
    c->empty = SEM_FAILED;
    c->full = SEM_FAILED;
    c->pid = -1;
    c->shm_name[0] = '\0';
    c->empty_name[0] = '\0';
    c->full_name[0] = '\0';
}

static int channel_create(channel_t *c, int id)
{
    pid_t me = getpid();

    snprintf(c->shm_name, NAME_LEN, "/lab3_%d_%d_shm", (int)me, id);
    snprintf(c->empty_name, NAME_LEN, "/lab3_%d_%d_empty", (int)me, id);
    snprintf(c->full_name, NAME_LEN, "/lab3_%d_%d_full", (int)me, id);

    c->fd = shm_open(c->shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (c->fd == -1) {
        perror("shm_open");
        c->shm_name[0] = '\0';
        return -1;
    }

    if (ftruncate(c->fd, sizeof(shm_buf_t)) == -1) {
        perror("ftruncate");
        return -1;
    }

    c->buf = mmap(NULL, sizeof(shm_buf_t), PROT_READ | PROT_WRITE, MAP_SHARED, c->fd, 0);
    if (c->buf == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    memset(c->buf, 0, sizeof(shm_buf_t));

    c->empty = sem_open(c->empty_name, O_CREAT | O_EXCL, 0600, 1);
    if (c->empty == SEM_FAILED) {
        perror("sem_open empty");
        c->empty_name[0] = '\0';
        return -1;
    }

    c->full = sem_open(c->full_name, O_CREAT | O_EXCL, 0600, 0);
    if (c->full == SEM_FAILED) {
        perror("sem_open full");
        c->full_name[0] = '\0';
        return -1;
    }

    return 0;
}

static void channel_destroy(channel_t *c)
{
    if (c->buf != MAP_FAILED) {
        munmap(c->buf, sizeof(shm_buf_t));
    }
    if (c->fd != -1) {
        close(c->fd);
    }
    if (c->empty != SEM_FAILED) {
        sem_close(c->empty);
    }
    if (c->full != SEM_FAILED) {
        sem_close(c->full);
    }
    if (c->empty_name[0] != '\0') {
        sem_unlink(c->empty_name);
    }
    if (c->full_name[0] != '\0') {
        sem_unlink(c->full_name);
    }
    if (c->shm_name[0] != '\0') {
        shm_unlink(c->shm_name);
    }
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

static int send_chunk(channel_t *c, const char *data, size_t len, int last, int stop)
{
    if (sem_wait_retry(c->empty) == -1) {
        return -1;
    }

    c->buf->len = len;
    c->buf->last = last;
    c->buf->stop = stop;
    if (len > 0) {
        memcpy(c->buf->data, data, len);
    }

    if (sem_post(c->full) == -1) {
        perror("sem_post");
        return -1;
    }
    return 0;
}

static int send_line(channel_t *c, const char *line, size_t len)
{
    size_t off = 0;

    do {
        size_t n = len - off;
        int last;

        if (n > BUF_SIZE) {
            n = BUF_SIZE;
        }

        last = (off + n == len);
        if (send_chunk(c, line + off, n, last, 0) == -1) {
            return -1;
        }
        off += n;
    } while (off < len);

    return 0;
}

static int spawn_child(channel_t *c, const char *filename)
{
    c->pid = fork();
    if (c->pid == -1) {
        perror("fork");
        return -1;
    }
    if (c->pid == 0) {
        execl("./child", "child", c->shm_name, c->empty_name, c->full_name, filename, (char *)NULL);
        perror("execl");
        _exit(127);
    }
    return 0;
}

static int read_filename(char **buf, size_t *cap, char **out)
{
    ssize_t n = getline(buf, cap, stdin);

    if (n == -1) {
        return -1;
    }
    if (n > 0 && (*buf)[n - 1] == '\n') {
        (*buf)[n - 1] = '\0';
    }
    *out = strdup(*buf);
    if (*out == NULL) {
        perror("strdup");
        return -1;
    }
    return 0;
}

static int feed_lines(channel_t *ch, char **line, size_t *cap)
{
    unsigned long idx = 0;
    ssize_t n;

    while ((n = getline(line, cap, stdin)) != -1) {
        size_t len = (size_t)n;
        channel_t *target;

        if (len > 0 && (*line)[len - 1] == '\n') {
            len--;
        }

        idx++;
        target = (idx % 2 == 1) ? &ch[0] : &ch[1];

        if (send_line(target, *line, len) == -1) {
            return -1;
        }
    }
    return 0;
}

static int finish_children(channel_t *ch)
{
    int status = EXIT_SUCCESS;
    int i;

    for (i = 0; i < 2; i++) {
        if (ch[i].pid > 0) {
            if (send_chunk(&ch[i], NULL, 0, 0, 1) == -1) {
                status = EXIT_FAILURE;
                kill(ch[i].pid, SIGTERM);
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if (ch[i].pid > 0) {
            int ws;
            if (waitpid(ch[i].pid, &ws, 0) == -1) {
                perror("waitpid");
                status = EXIT_FAILURE;
            } else if (!WIFEXITED(ws) || WEXITSTATUS(ws) != 0) {
                fprintf(stderr, "child%d завершился с ошибкой\n", i + 1);
                status = EXIT_FAILURE;
            }
        }
    }
    return status;
}

static int process(channel_t *ch, const char *name1, const char *name2, char **line, size_t *cap)
{
    int status = EXIT_SUCCESS;

    if (channel_create(&ch[0], 1) == -1 || channel_create(&ch[1], 2) == -1) {
        return EXIT_FAILURE;
    }

    if (spawn_child(&ch[0], name1) == -1 || spawn_child(&ch[1], name2) == -1) {
        status = EXIT_FAILURE;
    } else if (feed_lines(ch, line, cap) == -1) {
        status = EXIT_FAILURE;
    }

    if (finish_children(ch) != EXIT_SUCCESS) {
        status = EXIT_FAILURE;
    }
    return status;
}

int main(void)
{
    channel_t ch[2];
    char *name1 = NULL;
    char *name2 = NULL;
    char *line = NULL;
    size_t cap = 0;
    int status;

    channel_init(&ch[0]);
    channel_init(&ch[1]);

    if (read_filename(&line, &cap, &name1) == -1 ||
        read_filename(&line, &cap, &name2) == -1) {
        fprintf(stderr, "Не удалось прочитать имена файлов\n");
        free(name1);
        free(name2);
        free(line);
        return EXIT_FAILURE;
    }

    status = process(ch, name1, name2, &line, &cap);

    channel_destroy(&ch[0]);
    channel_destroy(&ch[1]);
    free(name1);
    free(name2);
    free(line);
    return status;
}