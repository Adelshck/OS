#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define REPEATS 5

typedef struct {
    const char *text;
    const char *pat;
    size_t pat_len;
    size_t from;
    size_t to;
    size_t count;
} task_t;

static pthread_mutex_t total_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t total_count = 0;

static void search_range(task_t *t)
{
    size_t i, j;
    for (i = t->from; i < t->to; i++) {
        for (j = 0; j < t->pat_len; j++) {
            if (t->text[i + j] != t->pat[j])
                break;
        }
        if (j == t->pat_len)
            t->count++;
    }
}

static void *worker(void *arg)
{
    task_t *t = arg;
    search_range(t);
    pthread_mutex_lock(&total_mutex);
    total_count += t->count;
    pthread_mutex_unlock(&total_mutex);
    return NULL;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    long sz;
    char *buf;
    if (!f) {
        perror("fopen");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    *len = fread(buf, 1, (size_t)sz, f);
    buf[*len] = '\0';
    fclose(f);
    return buf;
}

static size_t run_seq(const char *text, size_t n, const char *pat, size_t m)
{
    task_t t;
    memset(&t, 0, sizeof t);
    t.text = text;
    t.pat = pat;
    t.pat_len = m;
    t.to = n - m + 1;
    search_range(&t);
    return t.count;
}

static size_t run_par(const char *text, size_t n, const char *pat, size_t m,
                      unsigned nthreads)
{
    size_t positions = n - m + 1;
    size_t base = positions / nthreads;
    size_t rem = positions % nthreads;
    size_t cur = 0;
    unsigned i, created = 0;
    pthread_t *ids = malloc(nthreads * sizeof(pthread_t));
    task_t *tasks = calloc(nthreads, sizeof(task_t));
    size_t result;
    if (!ids || !tasks) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    total_count = 0;
    for (i = 0; i < nthreads; i++) {
        size_t len = base + (i < rem ? 1 : 0);
        tasks[i].text = text;
        tasks[i].pat = pat;
        tasks[i].pat_len = m;
        tasks[i].from = cur;
        tasks[i].to = cur + len;
        cur += len;
        if (pthread_create(&ids[i], NULL, worker, &tasks[i]) != 0) {
            perror("pthread_create");
            break;
        }
        created++;
    }
    for (i = 0; i < created; i++)
        pthread_join(ids[i], NULL);
    result = total_count;
    free(tasks);
    free(ids);
    return result;
}

int main(int argc, char **argv)
{
    unsigned nthreads;
    size_t n = 0, m, seq_count = 0, par_count = 0;
    char *text;
    double seq_ms = 0, par_ms = 0, t0, speedup;
    int r;

    if (argc != 4) {
        fprintf(stderr, "usage: %s FILE PATTERN THREADS\n", argv[0]);
        return 1;
    }
    nthreads = (unsigned)strtoul(argv[3], NULL, 10);
    if (nthreads == 0) {
        fprintf(stderr, "THREADS must be a positive number\n");
        return 1;
    }

    text = read_file(argv[1], &n);
    if (!text)
        return 1;
    m = strlen(argv[2]);
    if (m == 0 || m > n) {
        fprintf(stderr, "pattern length must be in 1..text length\n");
        return 1;
    }

    for (r = 0; r < REPEATS; r++) {
        t0 = now_ms();
        seq_count = run_seq(text, n, argv[2], m);
        seq_ms += now_ms() - t0;
    }
    seq_ms /= REPEATS;

    for (r = 0; r < REPEATS; r++) {
        t0 = now_ms();
        par_count = run_par(text, n, argv[2], m, nthreads);
        par_ms += now_ms() - t0;
    }
    par_ms /= REPEATS;

    speedup = seq_ms / par_ms;

    printf("pid=%d text=%zu pattern=%zu\n", (int)getpid(), n, m);
    printf("matches seq=%zu par=%zu %s\n", seq_count, par_count,
           seq_count == par_count ? "OK" : "MISMATCH");
    printf("threads=%u seq_ms=%.3f par_ms=%.3f speedup=%.3f efficiency=%.3f\n",
           nthreads, seq_ms, par_ms, speedup, speedup / nthreads);

    free(text);
    return 0;
}