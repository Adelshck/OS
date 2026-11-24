#include "commands.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_positive_int(const char *token, int *value)
{
    if (token == NULL) {
        return 0;
    }

    char *end;
    errno = 0;
    long parsed = strtol(token, &end, 10);
    if (errno == ERANGE || end == token || *end != '\0' || parsed < 1 || parsed > INT_MAX) {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

int read_command(char **line, size_t *capacity, struct lab_command *result, bool allow_switch)
{
    const char *whitespace = " \t\n\r\v\f";
    ssize_t length;

    while ((length = getline(line, capacity, stdin)) != -1) {
        if (memchr(*line, '\0', (size_t)length) != NULL) {
            fputs("Invalid input. Embedded NUL byte.\n", stderr);
            continue;
        }

        char *cursor;
        const char *command = strtok_r(*line, whitespace, &cursor);
        if (command == NULL) {
            continue;
        }

        if (allow_switch && strcmp(command, "0") == 0) {
            if (strtok_r(NULL, whitespace, &cursor) != NULL) {
                fputs("Expected 0 with no arguments.\n", stderr);
                continue;
            }
            *result = (struct lab_command){ .type = COMMAND_SWITCH };
        } else if (strcmp(command, "1") == 0) {
            int a, b;
            if (!parse_positive_int(strtok_r(NULL, whitespace, &cursor), &a) ||
                !parse_positive_int(strtok_r(NULL, whitespace, &cursor), &b) ||
                a > b || strtok_r(NULL, whitespace, &cursor) != NULL) {
                fprintf(stderr, "Expected 1 a b with 1 <= a <= b <= %d.\n", INT_MAX);
                continue;
            }
            *result = (struct lab_command){ .type = COMMAND_PRIME_COUNT, .first = a, .second = b };
        } else if (strcmp(command, "2") == 0) {
            int x;
            if (!parse_positive_int(strtok_r(NULL, whitespace, &cursor), &x) ||
                strtok_r(NULL, whitespace, &cursor) != NULL) {
                fprintf(stderr, "Expected 2 x with 1 <= x <= %d.\n", INT_MAX);
                continue;
            }
            *result = (struct lab_command){ .type = COMMAND_E, .first = x };
        } else {
            fputs(allow_switch ? "Unknown command. Use 0, 1 a b or 2 x.\n"
                               : "Unknown command. Use 1 a b or 2 x.\n", stderr);
            continue;
        }
        return 1;
    }

    if (ferror(stdin) || !feof(stdin)) {
        perror("getline");
        return -1;
    }
    return 0;
}
