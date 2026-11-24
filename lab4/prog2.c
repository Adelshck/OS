#include "commands.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

static const char *const library_paths[] = { "./libimpl1.so", "./libimpl2.so" };

struct loaded_library {
    void *handle;
    int (*prime_count)(int, int);
    float (*e)(int);
    int index;
};

static void *load_symbol(void *handle, const char *path, const char *name)
{
    dlerror();
    void *symbol = dlsym(handle, name);
    const char *error = dlerror();
    if (error != NULL || symbol == NULL) {
        fprintf(stderr, "%s: %s: %s\n", path, name, error != NULL ? error : "null symbol");
        return NULL;
    }
    return symbol;
}

static int load_library(int index, struct loaded_library *library)
{
    const char *path = library_paths[index];
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return 0;
    }

    /* POSIX permits casting a dlsym result to the matching function pointer. */
    struct loaded_library candidate = {
        .handle = handle,
        .prime_count = __extension__ (int (*)(int, int))load_symbol(handle, path, "prime_count"),
        .e = __extension__ (float (*)(int))load_symbol(handle, path, "e"),
        .index = index,
    };
    if (candidate.prime_count == NULL || candidate.e == NULL) {
        dlclose(handle);
        return 0;
    }

    *library = candidate;
    return 1;
}

static void switch_library(struct loaded_library *current)
{
    struct loaded_library next;
    if (load_library(1 - current->index, &next)) {
        dlclose(current->handle);
        *current = next;
    }
}

int main(void)
{
    struct loaded_library current;
    if (!load_library(0, &current)) {
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t capacity = 0;
    struct lab_command command;
    int status;

    while ((status = read_command(&line, &capacity, &command, true)) > 0) {
        switch (command.type) {
        case COMMAND_SWITCH:
            switch_library(&current);
            break;
        case COMMAND_PRIME_COUNT:
            printf("%d\n", current.prime_count(command.first, command.second));
            break;
        case COMMAND_E:
            printf("%.9g\n", (double)current.e(command.first));
            break;
        }
    }

    free(line);
    dlclose(current.handle);
    return status < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
