#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

#define BUFFER_SIZE 256

int read_line(char *buf, size_t max_len) {
    int bytes_read = read(STDIN_FILENO, buf, max_len - 1);
    if (bytes_read <= 0) {
        return 0;
    }
    buf[bytes_read] = '\0';
    if (bytes_read > 0 && buf[bytes_read - 1] == '\n') {
        buf[bytes_read - 1] = '\0';
    }
    return bytes_read;
}

int main() {
    char file1[BUFFER_SIZE];
    char file2[BUFFER_SIZE];

    if (read_line(file1, sizeof(file1)) <= 0) return 1;
    if (read_line(file2, sizeof(file2)) <= 0) return 1;

    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) return 1;

    if (pid1 == 0) {
        dup2(pipe1[0], STDIN_FILENO);

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        execl("./child", "./child", file1, NULL);
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) return 1;

    if (pid2 == 0) {
        dup2(pipe2[0], STDIN_FILENO);

        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        execl("./child", "./child", file2, NULL);
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[0]);

    char line_buf[BUFFER_SIZE];
    int bytes;
    int line_number = 1;

    while ((bytes = read(STDIN_FILENO, line_buf, sizeof(line_buf))) > 0) {
        if (line_number % 2 != 0) {
            write(pipe1[1], line_buf, bytes);
        } else {
            write(pipe2[1], line_buf, bytes);
        }
        line_number++;
    }

    close(pipe1[1]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}