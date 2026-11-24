#include "commands.h"
#include "contracts.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    struct lab_command command;
    int status;

    while ((status = read_command(&line, &capacity, &command, false)) > 0) {
        if (command.type == COMMAND_PRIME_COUNT) {
            printf("%d\n", prime_count(command.first, command.second));
        } else if (command.type == COMMAND_E) {
            printf("%.9g\n", (double)e(command.first));
        }
    }

    free(line);
    return status < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
