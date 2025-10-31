/*
 * Simple XOR encryption
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_KEY_SIZE 256
#define CHUNK_SIZE 4096

void
err_exit(char* reason)
{
    perror(reason);
    exit(EXIT_FAILURE);
}

void
get_key(char* dest) 
{
    struct termios oflags, nflags;
    tcgetattr(STDIN_FILENO, &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &nflags) != 0)
        err_exit("tcsetattr");

    printf("Enter key: ");
    fgets(dest, MAX_KEY_SIZE, stdin);
    putchar('\n');

    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0)
        err_exit("tcsetattr");
}

int 
main(int argc, char** argv) 
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s: <input file> <output file>\n", argv[0]);
        return 1;
    }

    char   key[MAX_KEY_SIZE];
    char   buf[CHUNK_SIZE];

    int in_fd, out_fd;
    int n;

    get_key(key);

    if ((in_fd = open(argv[1], O_RDONLY)) == -1)
        err_exit("open input");

    if ((out_fd = open(argv[2], O_WRONLY | O_CREAT, 0644)) == -1)
        err_exit("open output");

    while ((n = read(in_fd, buf, CHUNK_SIZE)) != 0) {
        if (n == -1) 
            err_exit("read");
            
        for (int i = 0; i < n; i++) {
            buf[i] ^= key[i % n];
        }

        write(out_fd, buf, n);
    }

    return 0;
}