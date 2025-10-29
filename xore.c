/*
 * Simple XOR encryption
 * TODO Encrypt filess
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_KEY_SIZE 256

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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s: <text>\n", argv[0]);
        return 1;
    }

    char   key[MAX_KEY_SIZE];
    char*  data = argv[1];
    size_t len = strlen(data);

    get_key(key);

    for (int i = 0; i < len; i++) {
        data[i] ^= key[i % len];
    }

    printf("%s\n", data);
    return 0;
}