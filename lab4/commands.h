#ifndef LAB4_COMMANDS_H
#define LAB4_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>

enum command_type {
    COMMAND_SWITCH,
    COMMAND_PRIME_COUNT,
    COMMAND_E,
};

struct lab_command {
    enum command_type type;
    int first;
    int second;
};

/* Return 1 for a command, 0 at end of input, or -1 on a read error. */
int read_command(char **line, size_t *capacity, struct lab_command *result, bool allow_switch);

#endif
