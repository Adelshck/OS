#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int is_vowel(char c) {
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
            c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        _exit(1);
    }


    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) {
        _exit(1);
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        close(fd);
        _exit(1);
    }
    close(fd); 

    char ch;
    while (read(STDIN_FILENO, &ch, 1) > 0) {
        if (!is_vowel(ch)) {
            write(STDOUT_FILENO, &ch, 1);
        }
    }

    return 0;
}